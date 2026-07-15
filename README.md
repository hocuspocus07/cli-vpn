# 🛡️ VPN Obfuscator CLI (WIP)

![Status: Work In Progress](https://img.shields.io/badge/Status-Work_In_Progress-orange)
![Language: C++17](https://img.shields.io/badge/Language-C++17-blue)
![Platform: Cross-Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows-lightgrey)

Hey! Welcome to my VPN Obfuscator CLI project. 

To be clear on what this project is: This does not implement a cryptographic VPN protocol from scratch. Cryptography is delegated to battle-tested tools like OpenVPN and WireGuard.

Instead, this project is a Systems Orchestration Daemon and Network Manager. It bridges the gap between raw cryptographic binaries and the operating system's kernel, providing automated provisioning, kernel-level routing, connection resilience, and obfuscation pipelines.

I started building this because I wanted a **100% free, system-wide VPN** that I could control entirely from the terminal, while also learning how low-level OS networking and C++ process management actually work under the hood. 

This project tackles:

1. Kernel-Level Routing: Modifying the OS routing tables directly using Linux Netlink sockets / Windows IP Helper API (Strictly no system("ip route") calls).

2. Process Supervision: Acting as a daemon that monitors VPN worker processes, capturing output, and automatically restarting them upon failure.

3. Fail-Safe Security: Implementing a true Network Kill Switch that blocks unencrypted outbound traffic if the managed tunnel collapses.

4. Obfuscation Integration: Programmatically integrating TLS-based obfuscation transports (such as stunnel) into an automated VPN provisioning pipeline to bypass Deep Packet Inspection.
## 🚧 Current Status (Work In Progress)

This is a live learning project. I am intentionally building it from scratch in phases to bridge the gap between high-level application code and low-level kernel routing. 

**My Roadmap:**
This is a live learning project built in phases, progressively moving deeper into OS internals:

- [x] Phase 1: Intelligent Data Pipeline: Memory-safe fetching and string-parsing of the VPN Gate Base64 CSV registry. (Planned: implementing a smart-scoring algorithm based on ping, uptime, and bandwidth).

- [x] Phase 2: Process Supervision: Using POSIX fork/exec and Windows CreateProcess to spawn, monitor, and gracefully terminate background binaries. Handling pipe streams and POSIX signals.

- [ ] Phase 3: Native Kernel Routing: Dropping shell commands to program directly against the OS network stack for route modification, metric priorities, and DNS state management.

- [ ] Phase 4: Resilience & Kill Switch: Implementing continuous connection monitoring (RTT/packet loss) and a network kill switch to prevent IP leakage during tunnel drops.

- [ ] Phase 5: Transport Obfuscation: Automating local proxy layers (like Stunnel) to wrap OpenVPN UDP signatures in standard TCP TLS handshakes.

## ⚙️ How it Works (Architecture)

Rather than writing custom cryptography from scratch (which is a bad idea), this C++ app acts as an intelligent **Network Coordinator**. 
1. It queries the API and allows the user to filter servers by country or ping.
2. It dynamically generates `.ovpn` configuration files in a temporary cache.
3. It launches standard tunneling binaries in the background.
4. It intercepts OS network routes to ensure *all* system traffic goes through the tunnel, not just browser traffic.

## 🛠️ Tech Stack & Dependencies
* **Core:** C++17
* **Build System:** CMake
* **Networking:** `cpp-httplib` (for API fetching)
* **Under the hood:** Designed to interface with standard `openvpn`, `wireguard`, and `stunnel` binaries.

---
*Note: Since this tool manipulates system routing tables and network interfaces, it requires Root/Administrator privileges to run the final executable.*