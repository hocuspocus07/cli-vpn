# 🛡️ VPN Obfuscator CLI (WIP)

![Status: Work In Progress](https://img.shields.io/badge/Status-Work_In_Progress-orange)
![Language: C++17](https://img.shields.io/badge/Language-C++17-blue)
![Platform: Cross-Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows-lightgrey)

Hey! Welcome to my VPN Obfuscator CLI project. 

I started building this because I wanted a **100% free, system-wide VPN** that I could control entirely from the terminal, while also learning how low-level OS networking and C++ process management actually work under the hood. 

Instead of relying on paid subscriptions, this tool programmatically hooks into the public [VPN Gate](https://www.vpngate.net/) academic registry, scrapes volunteer-hosted endpoints globally, and dynamically provisions a secure tunnel.

More importantly, I'm building this to experiment with bypassing strict network firewalls and Deep Packet Inspection (DPI). Standard VPN protocols are easy to block; this tool aims to encapsulate those handshakes inside obfuscated TLS traffic so it just looks like normal web browsing.

## 🚧 Current Status (Work In Progress)

This is a live learning project. I am intentionally building it from scratch in phases to bridge the gap between high-level application code and low-level kernel routing. 

**My Roadmap:**
- [x] **Phase 1: Data Pipeline:** Memory-safe fetching and string-parsing of the VPN Gate Base64 CSV registry using `cpp-httplib`.
- [ ] **Phase 2: Process Control:** Safely spawning, monitoring, and killing background binaries (like `openvpn` or `stunnel`) using OS-level APIs.
- [ ] **Phase 3: System Routing:** Writing elevated C++ wrappers to modify the default OS gateway (`0.0.0.0/0`) and force traffic through virtual TUN/TAP adapters.
- [ ] **Phase 4: Obfuscation Layer:** Implementing TLS-wrapping to hide OpenVPN UDP signatures from DPI firewalls.
- [ ] **Phase 5: Terminal UI:** Polishing the command-line interface for a seamless user experience.

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