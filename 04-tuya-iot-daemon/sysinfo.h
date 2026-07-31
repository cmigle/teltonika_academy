#ifndef SYSINFO_H
#define SYSINFO_H

#include <netinet/in.h>  // for INET_ADDRSTRLEN

#define MAX_INTERFACES 16

struct iface_info {
    char name[64];
    char ip[INET_ADDRSTRLEN];
    char netmask[INET_ADDRSTRLEN];
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
};

struct system_info {
    unsigned long long total_ram_bytes;
    unsigned long long free_ram_bytes;
    long uptime_seconds;
    double cpu_usage_percent;
    struct iface_info interfaces[MAX_INTERFACES];
    int interface_count;
};

// Fills `out` with a fresh snapshot of RAM, uptime, network interfaces
// (excluding loopback), and CPU usage (measured over a 1-second window,
// so this function blocks for about 1 second). Returns 0 on success,
// -1 on failure.
int collect_system_info(struct system_info *out);

#endif
