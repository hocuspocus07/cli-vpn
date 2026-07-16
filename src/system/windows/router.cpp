#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <iostream>
#include <vector>
#include "os_router.hpp"

using namespace std;

namespace VPNClient
{
    bool OSRouter::add_route(const string &destination, const string &mask, const string &gateway, DWORD interface_index, DWORD metric)
    {
        MIB_IPFORWARDROW row;
        ZeroMemory(&row, sizeof(MIB_IPFORWARDROW));

        // 1. Convert the std::string IP addresses into raw network bytes
        inet_pton(AF_INET, destination.c_str(), &row.dwForwardDest);
        inet_pton(AF_INET, mask.c_str(), &row.dwForwardMask);
        inet_pton(AF_INET, gateway.c_str(), &row.dwForwardNextHop);
        // 2. Set the remaining routing parameters
        row.dwForwardIfIndex = interface_index;
        row.dwForwardMetric1 = metric;

        // MIB_IPPROTO_NETMGMT tells the Windows Kernel that this route was added
        // manually by a network management software (your daemon).
        row.dwForwardProto = MIB_IPPROTO_NETMGMT;

        // 3. Execute the kernel modification
        // TODO: Call CreateIpForwardEntry(&row)
        DWORD result = CreateIpForwardEntry(&row);
        if (result == NO_ERROR)
        {
            cout << "Route added successfully\n";
            return true;
        }
        else
        {
            cerr << "Failed to add route. Error code:" << result << "\n";
            return false;
        }
    }
    bool OSRouter::delete_route(const string &destination, const string &mask, const string &gateway, DWORD interface_index)
    {
        MIB_IPFORWARDROW row;
        ZeroMemory(&row, sizeof(MIB_IPFORWARDROW));

        // 1. Convert the std::string IP addresses into raw network bytes
        inet_pton(AF_INET, destination.c_str(), &row.dwForwardDest);
        inet_pton(AF_INET, mask.c_str(), &row.dwForwardMask);
        inet_pton(AF_INET, gateway.c_str(), &row.dwForwardNextHop);

        row.dwForwardIfIndex = interface_index;
        row.dwForwardProto = MIB_IPPROTO_NETMGMT;

        // 2. Execute the kernel modification
        DWORD result = DeleteIpForwardEntry(&row);
        if (result == NO_ERROR)
        {
            cout << "Route deleted successfully\n";
            return true;
        }
        else
        {
            cerr << "Failed to delete route. Error code:" << result << "\n";
            return false;
        }
    }

    std::optional<VPNInterface> OSRouter::find_openvpn_adapter()
    {
        ULONG bufferSize = 0;

        // First call asks Windows how much memory we need.
        DWORD result = GetAdaptersAddresses(
            AF_INET,
            GAA_FLAG_INCLUDE_PREFIX,
            nullptr,
            nullptr,
            &bufferSize);

        if (result != ERROR_BUFFER_OVERFLOW)
        {
            std::cerr << "GetAdaptersAddresses failed.\n";
            return std::nullopt;
        }

        std::vector<BYTE> buffer(bufferSize);

        IP_ADAPTER_ADDRESSES *adapters =
            reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buffer.data());

        result = GetAdaptersAddresses(
            AF_INET,
            GAA_FLAG_INCLUDE_PREFIX,
            nullptr,
            adapters,
            &bufferSize);

        if (result != NO_ERROR)
        {
            std::cerr << "GetAdaptersAddresses failed: "
                      << result << '\n';
            return std::nullopt;
        }

        for (IP_ADAPTER_ADDRESSES *adapter = adapters;
             adapter != nullptr;
             adapter = adapter->Next)
        {
            std::wstring friendly(adapter->FriendlyName);

            // Convert UTF16 -> std::string
            std::string adapterName(
                friendly.begin(),
                friendly.end());

            // Accept both TAP and Wintun adapters
            if (adapterName.find("OpenVPN") == std::string::npos &&
                adapterName.find("TAP") == std::string::npos &&
                adapterName.find("Wintun") == std::string::npos)
            {
                continue;
            }

            VPNInterface vpn;

            vpn.interface_index = adapter->IfIndex;
            vpn.name = adapterName;

            //-----------------------
            // IPv4 Address
            //-----------------------

            if (adapter->FirstUnicastAddress)
            {
                sockaddr_in *ipv4 =
                    reinterpret_cast<sockaddr_in *>(
                        adapter->FirstUnicastAddress->Address.lpSockaddr);

                char ip[INET_ADDRSTRLEN];

                inet_ntop(
                    AF_INET,
                    &ipv4->sin_addr,
                    ip,
                    sizeof(ip));

                vpn.ip = ip;
            }

            //-----------------------
            // Gateway
            //-----------------------

            if (adapter->FirstGatewayAddress)
            {
                sockaddr_in *gateway =
                    reinterpret_cast<sockaddr_in *>(
                        adapter->FirstGatewayAddress->Address.lpSockaddr);

                char gw[INET_ADDRSTRLEN];

                inet_ntop(
                    AF_INET,
                    &gateway->sin_addr,
                    gw,
                    sizeof(gw));

                vpn.gateway = gw;
            }

            std::cout << "[INFO] VPN Adapter Found\n";
            std::cout << " Name      : " << vpn.name << '\n';
            std::cout << " Interface : " << vpn.interface_index << '\n';
            std::cout << " IP        : " << vpn.ip << '\n';
            std::cout << " Gateway   : " << vpn.gateway << '\n';

            return vpn;
        }

        return std::nullopt;
    }
}