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
std::atomic<bool> g_shutdown_requested(false);

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

BOOL WINAPI console_ctrl_handler(DWORD ctrlType)
{
    if (ctrlType == CTRL_C_EVENT ||
        ctrlType == CTRL_CLOSE_EVENT ||
        ctrlType == CTRL_BREAK_EVENT ||
        ctrlType == CTRL_LOGOFF_EVENT ||
        ctrlType == CTRL_SHUTDOWN_EVENT)
    {
        g_shutdown_requested = true;
        return TRUE;
    }

    return FALSE;
}

void cleanup()
{
    cout << "\n\n[DAEMON] Caught shutdown signal (Ctrl+C). Cleaning up..." << endl;
    if (g_pm && g_pm->is_running())
    {
        g_pm->terminate();
    }
    cout << "[DAEMON] Safe exit complete. Internet restored." << endl;
}

int main()
{
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
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
    VPNClient::ProcessManager pm;
    VPNClient::OSRouter router;

    g_pm = &pm;
    g_router = &router;

    bool connected = false;
    std::string vpnGateway;
    for (auto &server : active_servers)
    {
        cout << "\n=====================================\n";
        cout << "Trying server: " << server.country_long
             << " (" << server.ip << ")\n";
        cout << "=====================================\n";

        // Decode config
        string raw_ovpn_text =
            decode_base64(server.openvpn_config_base64);

        ofstream config("tunnel.ovpn");
        config << raw_ovpn_text;
        config.close();

        atomic<bool> tunnel_ready(false);

        bool launched =
            pm.launch(
                "openvpn --config tunnel.ovpn --route-noexec",
                [&tunnel_ready, &vpnGateway](const string &output)
                {
                    cout << "[OPENVPN] " << output << endl;

                    size_t pos = output.find("route-gateway ");

                    if (pos != std::string::npos)
                    {
                        pos += strlen("route-gateway ");

                        size_t end = output.find(',', pos);

                        vpnGateway =
                            output.substr(pos, end - pos);

                        std::cout
                            << "[INFO] VPN Gateway: "
                            << vpnGateway
                            << std::endl;
                    }

                    if (output.find(
                            "Initialization Sequence Completed") != string::npos)
                    {
                        tunnel_ready = true;
                    }
                });

        if (!launched)
        {
            continue;
        }

        auto start =
            chrono::steady_clock::now();

        while (!tunnel_ready && pm.is_running())
        {
            if (chrono::steady_clock::now() - start >
                chrono::seconds(15))
            {
                cout << "[TIMEOUT]\n";

                pm.terminate();

                break;
            }

            Sleep(100);
        }

        if (tunnel_ready)
        {
            connected = true;

            cout << "\nConnected successfully!\n";

            break;
        }
    }
    if (!connected)
    {
        cerr << "\nNo VPN server accepted the connection.\n";
        return 1;
    }
    auto adapter = router.find_openvpn_adapter();

    if (!adapter)
    {
        std::cerr << "Failed to locate VPN adapter\n";
        return 1;
    }

    adapter->gateway = vpnGateway;

    g_adapter = &(*adapter);

    std::cout << "\nFinal VPN Information\n";
    std::cout << "Interface : "
              << adapter->interface_index
              << '\n';

    std::cout << "IP        : "
              << adapter->ip
              << '\n';

    std::cout << "Gateway   : "
              << adapter->gateway
              << '\n';

    // Block the main thread while the lambda callback listens in the background
    if (!pm.is_running())
    {
        cerr << "[FATAL] VPN process crashed during startup." << endl;
        return 1;
    }

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
    while (pm.is_running() && !g_shutdown_requested)
    {
        Sleep(100);
    }

    if (g_shutdown_requested)
    {
        cleanup();
        return 0;
    }

    // If we break out of the loop without the user pressing Ctrl+C, something went wrong
    cout << "\n[DAEMON] VPN worker stopped unexpectedly. Triggering fail-safe shutdown." << endl;
    cleanup();

    return 0;
}