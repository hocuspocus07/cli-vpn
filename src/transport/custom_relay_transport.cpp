#include "custom_relay_transport.hpp"
#include <regex>
#include <iostream>

extern std::string decode_base64(const std::string &base64_str);

namespace VPNClient {

    CustomRelayTransport::~CustomRelayTransport() {
        stop();
    }

    bool CustomRelayTransport::prepare(const VpnServer& server, std::string& out_ovpn_config) {
        std::string raw_config = decode_base64(server.openvpn_config_base64);

        // 1. Extract the actual target IP and Port before we overwrite it
        std::smatch match;
        std::regex remote_regex(R"(remote\s+([0-9\.]+)\s+([0-9]+))");
        
        if (std::regex_search(raw_config, match, remote_regex) && match.size() == 3) {
            target_ip = match[1].str();
            target_port = match[2].str();
        } else {
            std::cerr << "[ERROR] Could not parse remote IP/Port from config.\n";
            return false;
        }

        // 2. Rewrite the OpenVPN Config to point to our local C++ socket
        out_ovpn_config = std::regex_replace(raw_config, remote_regex, "remote 127.0.0.1 8443");

        // Force TCP for our relay
        std::regex proto_udp(R"(proto\s+udp)");
        out_ovpn_config = std::regex_replace(out_ovpn_config, proto_udp, "proto tcp");

        return true;
    }

    bool CustomRelayTransport::start() {
        running = true;
        
        // Ensure Winsock is initialized
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        // 1. Create the local listening socket
        listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        sockaddr_in local_addr{};
        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(8443);
        inet_pton(AF_INET, "127.0.0.1", &local_addr.sin_addr);

        bind(listen_socket, (sockaddr*)&local_addr, sizeof(local_addr));
        listen(listen_socket, 1);

        std::cout << "[RELAY] Listening on 127.0.0.1:8443 for OpenVPN...\n";

        // 2. Launch the background thread to wait for OpenVPN
        accept_thread = std::thread(&CustomRelayTransport::accept_loop, this);
        return true;
    }

    void CustomRelayTransport::accept_loop() {
        // 1. Wait for OpenVPN to connect to us
        local_client = accept(listen_socket, nullptr, nullptr);
        if (!running || local_client == INVALID_SOCKET) return;

        std::cout << "[RELAY] OpenVPN connected. Dialing remote server " << target_ip << ":" << target_port << "...\n";

        // 2. Dial the actual VPN Gate Server
        remote_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        sockaddr_in remote_addr{};
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(std::stoi(target_port));
        inet_pton(AF_INET, target_ip.c_str(), &remote_addr.sin_addr);

        if (connect(remote_server, (sockaddr*)&remote_addr, sizeof(remote_addr)) == SOCKET_ERROR) {
            std::cerr << "[RELAY] Failed to connect to VPN Gate server.\n";
            stop();
            return;
        }

        std::cout << "[RELAY] Connection established. Starting byte shovel...\n";

        // 3. Launch the two directional relay threads
        c2s_thread = std::thread(relay_worker, local_client, remote_server, std::ref(running));
        s2c_thread = std::thread(relay_worker, remote_server, local_client, std::ref(running));
    }

    void CustomRelayTransport::relay_worker(SOCKET src, SOCKET dst, std::atomic<bool>& is_running) {
        char buffer[8192];
        while (is_running) {
            int bytes_read = recv(src, buffer, sizeof(buffer), 0);
            if (bytes_read <= 0) break; // Connection closed or error
            
            send(dst, buffer, bytes_read, 0);
        }
        // If one side drops, trigger the shutdown flag to kill the other thread
        is_running = false; 
    }

    bool CustomRelayTransport::is_healthy() {
        return running;
    }

    void CustomRelayTransport::stop() {
        running = false;
        
        if (listen_socket != INVALID_SOCKET) closesocket(listen_socket);
        if (local_client != INVALID_SOCKET) closesocket(local_client);
        if (remote_server != INVALID_SOCKET) closesocket(remote_server);

        if (accept_thread.joinable()) accept_thread.join();
        if (c2s_thread.joinable()) c2s_thread.join();
        if (s2c_thread.joinable()) s2c_thread.join();
    }

    std::string CustomRelayTransport::get_name() const {
        return "CUSTOM_RELAY";
    }
}