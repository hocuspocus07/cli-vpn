#ifndef PROCESS_MGR_HPP
#define PROCESS_MGR_HPP

#include <windows.h>
#include <string>
#include <functional>
#include <thread>

namespace VPNClient {

    class ProcessManager {
    public:
        ProcessManager();
        ~ProcessManager();

        // Launches the binary. 
        // We use a callback function (std::function) so the manager can 
        // send the intercepted output lines back to the main app in real-time.
        bool launch(const std::string& command, std::function<void(const std::string&)> on_output);

        // Kills the process using TerminateProcess
        void terminate();

        // Uses WaitForSingleObject to check if the child is still alive
        bool is_running() const;

    private:
        HANDLE hProcess;
        HANDLE hThread;
        HANDLE hReadPipe;
        
        // Internal helper to read the pipe without blocking
        void monitor_output(std::function<void(const std::string&)> on_output);
        std::thread monitorThread;
    };

} 

#endif