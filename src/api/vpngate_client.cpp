#include "api_client.hpp"

// #define CPPHTTPLIB_OPENSSL_SUPPORT 
#include "third_party/cpp-httplib/httplib.h"

#include <iostream>
#include <sstream>
#include <algorithm>
using namespace std;

namespace VPNClient{
    vector<VpnServer>VpnGateClient::fetch_servers(){
        vector<VpnServer> server_list;
        httplib::Client cli("http://www.vpngate.net");
        cout<<"[INFO] Fetching server list..."<<endl;

        auto res=cli.Get("/api/iphone/");

        if(!res||res->status!=200){
            cerr<<"[ERROR] Failed to fetch server list, status code: " 
                << (res ? res->status : 0) << endl;
                return server_list;
        }

        stringstream ss(res->body);
        string line;
        int line_count = 0;
        
        while(getline(ss,line)){
            line_count++;
//skip descriptive headers and column titles
            if(line_count<=2)continue;
//stopparsing if we reach end of data marker
            if(line.rfind("*",0)==0||line.empty())continue;

            //parse csv into vector of strings

            vector<string>row_tokens=split_csv_line(line);
            if(row_tokens.size()<15)continue;

            try{
                VpnServer server;
                server.ip=row_tokens[1];
                server.country_long=row_tokens[5];
                server.country_short=row_tokens[6];

                //convert strings to numeric types
                server.ping=stol(row_tokens[3]);
                server.speed=stol(row_tokens[4]);

                server.openvpn_config_base64=row_tokens[14];

                server_list.push_back(server);
            }catch(const exception&e){
            continue;
        }
        }

        cout<<"[SUCCESS] Successfully parsed "<<server_list.size()<<" active profiles."<<endl;
        return server_list;
    };

    vector<string>VpnGateClient::split_csv_line(string&line){
        vector<string>tokens;
        stringstream ss(line);
        string token;

        while(getline(ss,token,',')){
            //remove carriage returns or trailing spaces, if any
            token.erase(remove(token.begin(),token.end(),'\r'),token.end());
            tokens.push_back(token);
        }
        return tokens;
    }

    double get_score(const VpnServer&server){
        if (server.ping == -1) {
            return -1.0; 
        }
        
        return (server.speed / 1000.0) / (server.ping + 1.0);
    }
    void rank_servers(vector<VpnServer>&servers){
        sort(servers.begin(),servers.end(),[](const VpnServer&a,const VpnServer&b){
            return get_score(a) > get_score(b);
        });
    }
}