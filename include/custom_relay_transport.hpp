#ifndef CUSTOM_RELAY_TRANSPORT_HPP
#define CUSTOM_RELAY_TRANSPORT_HPP

#include "transport.hpp"
#include <string>
#include <thread>
#include <atomic>
#include <winsock2.h>
#include <ws2tcpip.h>

namespace VPNClient {

    class CustomRelayTransport : public ITransport {
    private:
        std::string target_ip;
        std::string target_port;
        
        SOCKET listen_socket = INVALID_SOCKET;
        SOCKET local_client = INVALID_SOCKET;
        SOCKET remote_server = INVALID_SOCKET;
        
        std::thread accept_thread;
        std::thread c2s_thread; // Client to Server
        std::thread s2c_thread; // Server to Client
        
        std::atomic<bool> running{false};

        // Internal background workers
        void accept_loop();
        static void relay_worker(SOCKET src, SOCKET dst, std::atomic<bool>& is_running);

    public:
        ~CustomRelayTransport();
        bool prepare(const VpnServer& server, std::string& out_ovpn_config) override;
        bool start() override;
        bool is_healthy() override;
        void stop() override;
        std::string get_name() const override;
    };

}

#endif