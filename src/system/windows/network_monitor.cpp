#include "network_monitor.hpp"
#include <iostream>
#include <iomanip>

using namespace std;

namespace VPNClient
{
    NetworkMonitor::~NetworkMonitor()
    {
        stop();
    }

    void NetworkMonitor::start(DWORD interface_index)
    {
        if_index = interface_index;

        if (is_running)
            return;

        is_running = true;
        worker_thread = thread(&NetworkMonitor::monitor_loop, this);
    }

    void NetworkMonitor::stop()
    {
        if (is_running)
        {
            is_running = false;
            if (worker_thread.joinable())
            {
                worker_thread.join();
            }
        }
    }

    void NetworkMonitor::monitor_loop()
    {
        ULONG64 prev_in = 0;
        ULONG64 prev_out = 0;

        ULONG64 start_in = 0;
        ULONG64 start_out = 0;
        bool first_run = true;

        while (is_running)
        {
            MIB_IFROW ifRow{};
            ifRow.dwIndex = if_index;
            if (GetIfEntry(&ifRow) == NO_ERROR)
            {
                if (first_run)
                {
                    start_in = prev_in = ifRow.dwInOctets;
                    start_out = prev_out = ifRow.dwOutOctets;
                    first_run = false;
                }
                else
                {
                    ULONG64 bytes_in_per_sec = ifRow.dwInOctets - prev_in;
                    ULONG64 bytes_out_per_sec = ifRow.dwOutOctets - prev_out;

                    double mb_down = static_cast<double>(bytes_in_per_sec) / (1024.0 * 1024.0);
                    double mb_up = static_cast<double>(bytes_out_per_sec) / (1024.0 * 1024.0);

                    ULONG64 session =
                        (ifRow.dwInOctets - start_in) +
                        (ifRow.dwOutOctets - start_out);
                    cout << "\r[TELEMETRY] Down: " << fixed << setprecision(2) << mb_down << " MB/s | "
                         << "Up: " << mb_up << " MB/s | "
                         << "Total Session: " << session / (1024.0 * 1024.0) << " MB"
                         << flush;
                }
                prev_in = ifRow.dwInOctets;
                prev_out = ifRow.dwOutOctets;
            }
            Sleep(1000);
        }
    }
}