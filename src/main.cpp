#include <iostream>
#include "api_client.hpp"
using namespace std;

int main() {
    cout << "=== VPN Obfuscator CLI Initialized ===" << endl;
    VPNClient::VpnGateClient client;

    vector<VPNClient::VpnServer>active_servers=client.fetch_servers();

    if(active_servers.empty()){
        cerr<<"[ERROR] No active servers found. Check your connection."<<endl;
        return 1;
    }
    cout << "\n--- Sample of Retrieved Targets ---" << endl;
    for (const auto& server : active_servers) {
        cout << "Location: " << server.country_long 
             << " | IP: " << server.ip 
             << " | Ping: " << server.ping << "ms"
             << "\t| Speed: " << (server.speed / 1024 / 1024) << " Mbps" << endl;
    }
    return 0;
}