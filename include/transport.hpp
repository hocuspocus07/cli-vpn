#ifndef TRANSPORT_HPP
#define TRANSPORT_HPP

#include <string>
#include "api_client.hpp" // For VpnServer struct

namespace VPNClient {

    class ITransport {
    public:
        virtual ~ITransport() = default;

        virtual bool prepare(const VpnServer& server, std::string& out_ovpn_config) = 0;

        virtual bool start() = 0;

        virtual bool is_healthy() = 0;

        virtual void stop() = 0;
        
        virtual std::string get_name() const = 0; 
    };

}

#endif