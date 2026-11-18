# Teltonika IoT Academy: embedded Linux projects

Projects from the [Teltonika Networks](https://teltonika-networks.com/) IoT Academy, a 335-hour internship in embedded software (July to October 2026). The assignments start with C fundamentals, move on to services running on a Teltonika RUTX10 industrial router (RutOS, which is based on OpenWrt), and end with a patch for an open bug in OpenWrt's network daemon.

A Teltonika engineer code-reviewed every assignment, and most went through several rounds of fixes before they were accepted.

## Projects

| | Project | Summary | Stack |
|---|---------|---------|-------|
| 01 | [C debugging](01-c-debugging) | Found and fixed the bugs in 11 broken C programs: buffer overflows, a double free, uninitialised pointers, file handling | C |
| 02 | [Address book](02-address-book) | Modular terminal address book built on a linked list, with CSV persistence and saving on exit or signal | C, Make |
| 03 | [Shared library](03-shared-library) | The linked list moved into `liblinklist.so`, with recursive Makefiles, rpath and install targets | C, Make |
| 04 | [Tuya IoT daemon](04-tuya-iot-daemon) | Daemon that connects a router to the Tuya cloud over MQTT/TLS and carries out cloud commands through ubus | C, MQTT, ubus, UCI |
| 05 | [OpenVPN monitor](05-openvpn-monitor) | RutOS service that lists connected VPN clients and disconnects them over ubus, one object per server | Lua, ubus, UCI, procd |
| 06 | [netifd route patch](06-netifd-route-patch) | Root-cause analysis and fix for [OpenWrt issue #10791](https://github.com/openwrt/openwrt/issues/10791) | C, netifd, Linux routing |

Each folder has its own README with build steps, design decisions and the problems solved along the way.

The academy also covered Linux basics, networking fundamentals (router setup, static addressing, SSH/SCP) and building RutOS firmware from Teltonika's GPL SDK. That work was environment and configuration setup with no standalone code, so it isn't included here.

## Skills

- **C systems programming:** manual memory management, linked data structures, signals, daemons, syslog, Make, shared libraries
- **OpenWrt / RutOS:** ubus (from C and Lua), UCI, procd init scripts, hotplug, cross-compiling with the RutOS SDK
- **Networking:** OpenVPN (TLS, PKI, management interface), static routes and the kernel routing table, MQTT over TLS
- **Debugging on real hardware:** working from logs and testing one hypothesis at a time, then retesting with multiple servers and clients
- **Workflow:** code-review iterations, upstream-style patches with `git format-patch`

## Environment

- Teltonika RUTX10 (quad-core ARM Cortex-A7) running RutOS
- RutOS SDK `RUTX_R_GPL_00.07.13.4` on an Ubuntu workstation

## Repository layout

```
01-c-debugging/          C exercises, one file per bug
02-address-book/         linked-list address book + Makefile
03-shared-library/       lib/ (liblinklist.so) and src/ (program)
04-tuya-iot-daemon/      Tuya MQTT <-> ubus daemon
05-openvpn-monitor/      router filesystem layout: usr/bin, etc/init.d, etc/hotplug.d
06-netifd-route-patch/   git format-patch output + write-up
```

Each project has its own branch, merged into `main`. The work was done in Teltonika's GitLab and moved here afterwards, so project commits carry the dates the work was done. Documentation and later fixes are on the `cleanup` branch.

## Author

Miglė Cirtautaitė, Computer Science student at Kaunas University of Technology.
