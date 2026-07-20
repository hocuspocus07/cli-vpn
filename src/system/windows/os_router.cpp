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

        in_addr addr{};

        // 1. Build the IP addresses first
        inet_pton(AF_INET, destination.c_str(), &addr);
        row.dwForwardDest = addr.S_un.S_addr;

        inet_pton(AF_INET, mask.c_str(), &addr);
        row.dwForwardMask = addr.S_un.S_addr;

        inet_pton(AF_INET, gateway.c_str(), &addr);
        row.dwForwardNextHop = addr.S_un.S_addr;

        // 2. Set the routing parameters
        row.dwForwardIfIndex = interface_index;
        row.dwForwardMetric1 = get_interface_metric(interface_index) + metric;
        
        // Windows strictly requires unused metrics to be set to -1 
        row.dwForwardMetric2 = (DWORD)-1;
        row.dwForwardMetric3 = (DWORD)-1;
        row.dwForwardMetric4 = (DWORD)-1;
        row.dwForwardMetric5 = (DWORD)-1;

        row.dwForwardType = MIB_IPROUTE_TYPE_INDIRECT;
        row.dwForwardProto = MIB_IPPROTO_NETMGMT;
        row.dwForwardAge = 0;

        // 3. Defensive Deletion: Now that the row is fully populated, clear any zombies
        DeleteIpForwardEntry(&row); 

        // 4. Execute the kernel modification
        DWORD result = CreateIpForwardEntry(&row);
        
        if (result == NO_ERROR)
        {
            cout << "Route added successfully\n";
            return true;
        }
        else if (result == ERROR_OBJECT_ALREADY_EXISTS) // Error 5010
        {
            result = SetIpForwardEntry(&row);
            if (result == NO_ERROR)
            {
                cout << "Existing route updated successfully\n";
                return true;
            }
            else
            {
                cerr << "Failed to update existing route. Error code:" << result << "\n";
                return false;
            }
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

        in_addr addr{};

        // 1. Convert the std::string IP addresses into raw network bytes
        inet_pton(AF_INET, destination.c_str(), &addr);
        row.dwForwardDest = addr.S_un.S_addr;

        inet_pton(AF_INET, mask.c_str(), &addr);
        row.dwForwardMask = addr.S_un.S_addr;

        inet_pton(AF_INET, gateway.c_str(), &addr);
        row.dwForwardNextHop = addr.S_un.S_addr;

        row.dwForwardIfIndex = interface_index;
        row.dwForwardMetric1 = get_interface_metric(interface_index) + 1; 
        
        row.dwForwardMetric2 = (DWORD)-1;
        row.dwForwardMetric3 = (DWORD)-1;
        row.dwForwardMetric4 = (DWORD)-1;
        row.dwForwardMetric5 = (DWORD)-1;

        row.dwForwardType = MIB_IPROUTE_TYPE_INDIRECT;
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

        auto *adapters =
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

        for (auto *adapter = adapters;
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
                auto *ipv4 =
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

            return vpn;
        }

        return std::nullopt;
    }

    DWORD OSRouter::get_interface_metric(DWORD ifIndex)
    {
        MIB_IPINTERFACE_ROW iface;
        InitializeIpInterfaceEntry(&iface);

        iface.Family = AF_INET;
        iface.InterfaceIndex = ifIndex;

        if (GetIpInterfaceEntry(&iface) != NO_ERROR)
            return 300; // fallback
        return iface.Metric;
    }
}