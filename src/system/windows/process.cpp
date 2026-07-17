#include "process_mgr.hpp"
#include <iostream>
#include <string>
#include <chrono>
#include <vector>

namespace VPNClient
{
    ProcessManager::ProcessManager()
        : hProcess(NULL), hThread(NULL), hReadPipe(NULL)
    {
    }

    ProcessManager::~ProcessManager()
    {
        if (monitorThread.joinable())
            monitorThread.join();
    }
    bool ProcessManager::launch(const std::string &command, std::function<void(const std::string &)> on_output)
    {
        if (monitorThread.joinable())
        {
            monitorThread.join();
        }
        HANDLE hWritePipe = NULL;
        SECURITY_ATTRIBUTES saAttr;

        // Set the bInheritHandle flag so pipe handles are inherited by the child.
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        // 1. Create a pipe for the child process's STDOUT.
        if (!CreatePipe(&this->hReadPipe,
                        &hWritePipe,
                        &saAttr,
                        0))
        {
            std::cerr << "Error: CreatePipe failed.\n";
            return 1;
        }

        // Ensure the read handle to the pipe is NOT inherited.
        SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

        PROCESS_INFORMATION piProcInfo;
        STARTUPINFOA siStartInfo;
        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
        ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));

        // 2. Set up members of the STARTUPINFO structure.
        // This structure specifies the STDIN and STDOUT handles for redirection.
        siStartInfo.cb = sizeof(STARTUPINFO);
        siStartInfo.hStdError = hWritePipe;
        siStartInfo.hStdOutput = hWritePipe;
        siStartInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

        std::vector<char> cmd(command.begin(), command.end());
        cmd.push_back('\0');

        BOOL success = CreateProcessA(
            nullptr,
            cmd.data(),
            nullptr,
            nullptr,
            TRUE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &siStartInfo,
            &piProcInfo);
        if (!success)
            return false;

        this->hProcess = piProcInfo.hProcess;
        this->hThread = piProcInfo.hThread;
        this->hReadPipe = hReadPipe;

        // Parent doesn't write to the pipe.
        CloseHandle(hWritePipe);
        this->hReadPipe = hReadPipe;
        // Start monitoring in the background.
        monitorThread = std::thread(
            &ProcessManager::monitor_output,
            this,
            on_output);

        return true;
    };

    void ProcessManager::terminate()
    {
        if (monitorThread.joinable())
        {
            monitorThread.join();
        }
        if (hProcess)
        {
            TerminateProcess(hProcess, 0);

            CloseHandle(hProcess);
            hProcess = NULL;
        }

        if (hThread)
        {
            CloseHandle(hThread);
            hThread = NULL;
        }

        if (hReadPipe)
        {
            CloseHandle(hReadPipe);
            hReadPipe = NULL;
        }

        std::cout << "Child terminated successfully\n";
    }

    bool ProcessManager::is_running() const
    {
        if (!hProcess)
            return false;
        DWORD exitCode;
        if (!GetExitCodeProcess(this->hProcess, &exitCode) || exitCode != STILL_ACTIVE)
        {
            return false;
        }
        return exitCode == STILL_ACTIVE;
;
    }

    void ProcessManager::monitor_output(
        std::function<void(const std::string &)> on_output)
    {
        char buffer[4096];
        DWORD bytesRead;
        std::string line;

        while (ReadFile(
            hReadPipe,
            buffer,
            sizeof(buffer) - 1,
            &bytesRead,
            nullptr))
        {
            if (bytesRead == 0)
                break;

            buffer[bytesRead] = '\0';

            line += buffer;

            size_t pos;
            while ((pos = line.find('\n')) != std::string::npos)
            {
                std::string outputLine = line.substr(0, pos);
                if (outputLine.empty())
                    continue;
                // Remove Windows '\r'
                if (!outputLine.empty() && outputLine.back() == '\r')
                    outputLine.pop_back();

                on_output(outputLine);

                line.erase(0, pos + 1);
            }
        }

        // Flush any remaining partial line.
        if (!line.empty())
            on_output(line);
    }
}