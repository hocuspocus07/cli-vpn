#ifndef OS_ROUTER_HPP
#define OS_ROUTER_HPP

#include <string>
#include <windows.h>
#include <iphlpapi.h>
#include <optional>


namespace VPNClient
{
    struct VPNInterface
        {
            DWORD interface_index;
            std::string gateway;
            std::string ip;
            std::string name;
        };

    class OSRouter
    {
    public:
        OSRouter() = default;
        ~OSRouter() = default;

        // Injects a new route into the Windows Kernel
        bool add_route(const std::string &destination, const std::string &mask, const std::string &gateway, DWORD interface_index, DWORD metric);

        // Removes a route from the Windows Kernel
        bool delete_route(const std::string &destination, const std::string &mask, const std::string &gateway, DWORD interface_index);
        
        std::optional<VPNInterface> find_openvpn_adapter();
    };

}

#endif