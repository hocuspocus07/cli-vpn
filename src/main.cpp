#include <iostream>
#include <future>
#include "api_client.hpp"
#include "ping_helper.hpp"
#include "process_mgr.hpp"
using namespace std;

int main() {
    cout << "=== VPN Obfuscator CLI Initialized ===" << endl;
    VPNClient::VpnGateClient client;

    vector<VPNClient::VpnServer>active_servers=client.fetch_servers();

    if(active_servers.empty()){
        cerr<<"[ERROR] No active servers found. Check your connection."<<endl;
        return 1;
    }
    //calculating true ping from local machine using multithreading
    vector<future<int>> ping_tasks;
    for (const auto& server : active_servers) {
        ping_tasks.push_back(
            async(launch::async, ping_server, server.ip)
        );
    }

    for (size_t i = 0; i < active_servers.size(); ++i) {
        // .get() forces the main program to pause until this specific thread finishes.
        int true_ping = ping_tasks[i].get(); 
        
        // Overwrite the fake API ping with real, native Windows ping
        active_servers[i].ping = true_ping; 
    }
    VPNClient::rank_servers(active_servers);
    cout << "\n--- Sorted list of Retrieved Targets based on score---" << endl;
    for (const auto& server : active_servers) {
        if (server.ping != -1) {
            cout << "Location: " << server.country_long 
                << " | IP: " << server.ip 
                << " | Ping: " << server.ping << "ms"
                << "\t| Speed: " << (server.speed / 1024 / 1024) << " Mbps" 
                << "\t| Score: " << VPNClient::get_score(server) << endl;
        }
    }

    VPNClient::ProcessManager pm;
    pm.launch("ping -t 8.8.8.8", [](const std::string& output) {
        std::cout << "[DAEMON LOG]: " << output << "\n";
    });
    std::cout << "\nPress ENTER to terminate the child...\n";
    std::cin.get();

    pm.terminate();
    return 0;
}