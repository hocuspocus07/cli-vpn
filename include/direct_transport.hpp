#ifndef DIRECT_TRANSPORT_HPP
#define DIRECT_TRANSPORT_HPP

#include "transport.hpp"
#include <iostream>
#include <string>

extern std::string decode_base64(const std::string &base64_str);

namespace VPNClient {

    class DirectTransport : public ITransport {
    public:
        bool prepare(const VpnServer& server, std::string& out_ovpn_config) override {
            out_ovpn_config = decode_base64(server.openvpn_config_base64);
            return !out_ovpn_config.empty();
        }

        bool start() override {
            return true; 
        }

        bool is_healthy() override {
            return true; 
        }

        void stop() override {
            // Nothing to clean up.
        }

        std::string get_name() const override {
            return "DIRECT";
        }
    };

}

#endif