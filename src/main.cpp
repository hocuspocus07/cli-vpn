#include <iostream>
#include <future>
#include <atomic>
#include <csignal>
#include <fstream>
#include <chrono>
#include <thread>

#include "api_client.hpp"
#include "ping_helper.hpp"
#include "process_mgr.hpp"
#include "os_router.hpp"
using namespace std;

VPNClient::ProcessManager *g_pm = nullptr;
VPNClient::OSRouter *g_router = nullptr;
VPNClient::VPNInterface *g_adapter = nullptr;

#include <wincrypt.h>
// IMPORTANT: If you aren't using CMake yet, uncomment the line below:
// #pragma comment(lib, "Crypt32.lib")

std::string decode_base64(const std::string &base64_str)
{
    DWORD decoded_length = 0;

    // Step 1: Ask Windows how big the decoded string will be
    CryptStringToBinaryA(base64_str.c_str(), 0, CRYPT_STRING_BASE64, NULL, &decoded_length, NULL, NULL);

    if (decoded_length == 0)
        return ""; // Decoding failed or string is empty

    // Step 2: Allocate memory and decode it
    std::vector<BYTE> buffer(decoded_length);
    CryptStringToBinaryA(base64_str.c_str(), 0, CRYPT_STRING_BASE64, buffer.data(), &decoded_length, NULL, NULL);

    // Convert the raw bytes back into a standard C++ string
    return std::string(buffer.begin(), buffer.end());
}

void handle_shutdown(int signum)
{
    cout << "\n\n[DAEMON] Caught shutdown signal (Ctrl+C). Cleaning up..." << endl;
    // if (g_router && g_adapter)
    // {
    //     g_router->delete_route(
    //         "0.0.0.0",
    //         "0.0.0.0",
    //         g_adapter->gateway,
    //         g_adapter->interface_index);
    // }
    if (g_pm && g_pm->is_running())
    {
        // Kill the OpenVPN background worker
        g_pm->terminate();
    }
    cout << "[DAEMON] Safe exit complete. Internet restored." << endl;
    exit(signum);
}

int main()
{
    signal(SIGINT, handle_shutdown);
    cout << "=== VPN Obfuscator CLI Initialized ===" << endl;
    VPNClient::VpnGateClient client;

    vector<VPNClient::VpnServer> active_servers = client.fetch_servers();

    if (active_servers.empty())
    {
        cerr << "[ERROR] No active servers found. Check your connection." << endl;
        return 1;
    }
    // calculating true ping from local machine using multithreading
    vector<future<int>> ping_tasks;
    for (const auto &server : active_servers)
    {
        ping_tasks.push_back(
            async(launch::async, ping_server, server.ip));
    }

    for (size_t i = 0; i < active_servers.size(); ++i)
    {
        // .get() forces the main program to pause until this specific thread finishes.
        int true_ping = ping_tasks[i].get();

        // Overwrite the fake API ping with real, native Windows ping
        active_servers[i].ping = true_ping;
    }
    VPNClient::rank_servers(active_servers);
    auto &best_server = active_servers[0];
    cout << "\n--- Sorted list of Retrieved Targets based on score---" << endl;
    for (const auto &server : active_servers)
    {
        if (server.ping != -1)
        {
            cout << "Location: " << server.country_long
                 << " | IP: " << server.ip
                 << " | Ping: " << server.ping << "ms"
                 << "\t| Speed: " << (server.speed / 1024 / 1024) << " Mbps"
                 << "\t| Score: " << VPNClient::get_score(server) << endl;
        }
    }
    // --- CONFIG EXTRACTION ---
    cout << "[DAEMON] Decoding and writing configuration file..." << endl;

    // Decode the Base64 string from your winning server
    // (Make sure 'openvpn_config_base64' matches whatever you named the struct variable in Phase 1)
    std::string raw_ovpn_text = decode_base64(best_server.openvpn_config_base64);

    ofstream config_file("tunnel.ovpn");
    config_file << raw_ovpn_text;
    // config_file << "\nroute-noexec\n";
    config_file.close();

    // --- INSTANTIATE CORE SYSTEMS ---
    VPNClient::ProcessManager pm;
    VPNClient::OSRouter router;

    // Assign to globals so our Ctrl+C handler can see them
    g_pm = &pm;
    g_router = &router;

    atomic<bool> tunnel_ready = false;

    cout << "[DAEMON] Spawning OpenVPN worker..." << endl;
    bool launched = pm.launch("openvpn --config tunnel.ovpn", [&tunnel_ready](const string &output)
                              {
        // Print the OpenVPN logs to the console
        cout << "[OPENVPN]: " << output << endl;
        
        // Listen for the magic words
        if (output.find("Initialization Sequence Completed") != string::npos) {
            tunnel_ready = true;
        } });
    if (!launched)
        return 1;

    // --- WAIT FOR HANDSHAKE ---
    cout << "[DAEMON] Waiting for cryptographic handshake..." << endl;

    // Block the main thread while the lambda callback listens in the background
    while (!tunnel_ready && pm.is_running())
    {
        Sleep(100);
    }

    if (!pm.is_running())
    {
        cerr << "[FATAL] VPN process crashed during startup." << endl;
        return 1;
    }

    auto adapter = router.find_openvpn_adapter();

    if (!adapter)
    {
        cerr << "Failed to locate VPN adapter\n";
        return 1;
    }

    g_adapter = &(*adapter);

    // --- PHASE 3: INJECT KERNEL ROUTE ---
    cout << "\n[DAEMON] Tunnel ready. Hijacking Windows network routes..." << endl;

    // if (router.add_route(
    //         "0.0.0.0",
    //         "0.0.0.0",
    //         adapter->gateway,
    //         adapter->interface_index,
    //         1))
    // {
    //     cout << "[SUCCESS] Traffic secured. Press Ctrl+C to disconnect." << endl;
    // }
    // else
    // {
    //     cerr << "[WARNING] Route injection failed! Traffic might leak." << endl;
    // }

    // --- PHASE 4: THE SUPERVISION LOOP ---
    // Keep the C++ program alive as long as OpenVPN is running
    while (pm.is_running())
    {
        Sleep(1000);
    }

    // If we break out of the loop without the user pressing Ctrl+C, something went wrong
    cout << "\n[DAEMON] VPN worker stopped unexpectedly. Triggering fail-safe shutdown." << endl;
    handle_shutdown(1);

    return 0;
}