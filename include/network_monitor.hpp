#ifndef NETWORK_MONITOR_HPP
#define NETWORK_MONITOR_HPP

#include <windows.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <atomic>
#include <thread>

namespace VPNClient {

    class NetworkMonitor {
    private:
        std::atomic<bool> is_running{false};
        std::thread worker_thread;
        DWORD if_index;

        void monitor_loop();

    public:
        NetworkMonitor() = default;
        ~NetworkMonitor();

        // Starts the background polling thread
        void start(DWORD interface_index);
        
        // Safely stops and joins the thread
        void stop();
    };

} 

#endif 