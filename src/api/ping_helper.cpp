#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <string>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

int ping_server(const std::string& ip_address) {
    HANDLE hIcmpFile;
    unsigned long ipaddr = INADDR_NONE;
    DWORD dwRetVal = 0;
    char SendData[] = "pingtest";
    LPVOID ReplyBuffer = NULL;
    DWORD ReplySize = 0;
    int final_ping = -1; // Default to -1 (server dead or timed out)

    // 1. Convert C++ string to Windows network IP address format
    ipaddr = inet_addr(ip_address.c_str());
    if (ipaddr == INADDR_NONE) {
        return -1;
    }

    hIcmpFile = IcmpCreateFile();
    if (hIcmpFile == INVALID_HANDLE_VALUE) {
        return -1;
    }

    // 2. Allocate memory for the reply buffer
    ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData) + 8;
    ReplyBuffer = (VOID*)malloc(ReplySize);
    if (ReplyBuffer == NULL) {
        IcmpCloseHandle(hIcmpFile); // Always close the handle
        return -1;
    }

    // 3. Send the Ping (with a 1000ms strict timeout)
    dwRetVal = IcmpSendEcho2(hIcmpFile, NULL, NULL, NULL,
                             ipaddr, SendData, sizeof(SendData), NULL,
                             ReplyBuffer, ReplySize, 1000);
    
    // 4. Extract the time if the ping was successful
    if (dwRetVal != 0) {
        PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
        final_ping = pEchoReply->RoundTripTime;
    }

    free(ReplyBuffer);
    IcmpCloseHandle(hIcmpFile);

    return final_ping;
}