#ifndef API_CLIENT_HPP
#define API_CLIENT_HPP

#include <string>
#include <vector>

namespace VPNClient{
    struct VpnServer{
        std::string ip;
        std::string country_long;
        std::string country_short;
        long speed;
        int ping;
        std::string openvpn_config_base64;
    };

    class VpnGateClient{
        public:
        VpnGateClient()=default;
        ~VpnGateClient()=default;

        std::vector<VpnServer> fetch_servers();
        private:
        std::vector<std::string>split_csv_line(std::string& line);
    };
}

#endif