#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include "sysinfo.h"

// ----------------- internal helpers (not exposed in sysinfo.h) -----------------

static int get_iface_bytes(const char *iface_name_wanted, unsigned long long *rx_out, unsigned long long *tx_out)
{
    FILE *netdev = fopen("/proc/net/dev", "r");
    if (netdev == NULL) {
        return -1;
    }

    char line[512];
    int line_number = 0;
    int found = 0;

    while (fgets(line, sizeof(line), netdev)) {
        line_number++;
        if (line_number <= 2) {
            continue;
        }

        char iface_name[64];
        unsigned long long rx_bytes = 0;
        unsigned long long tx_bytes = 0;

        sscanf(line, "%63[^:]: %llu %*u %*u %*u %*u %*u %*u %*u %llu",
               iface_name, &rx_bytes, &tx_bytes);

        char *trimmed_name = iface_name;
        while (*trimmed_name == ' ') {
            trimmed_name++;
        }

        if (strcmp(trimmed_name, iface_name_wanted) == 0) {
            *rx_out = rx_bytes;
            *tx_out = tx_bytes;
            found = 1;
            break;
        }
    }

    fclose(netdev);
    return found ? 0 : -1;
}

static int read_cpu_times(unsigned long long *user, unsigned long long *nice, unsigned long long *system,
                           unsigned long long *idle, unsigned long long *iowait, unsigned long long *irq,
                           unsigned long long *softirq)
{
    FILE *stat_file = fopen("/proc/stat", "r");
    if (stat_file == NULL) {
        return -1;
    }

    char line[256];
    if (fgets(line, sizeof(line), stat_file) == NULL) {
        fclose(stat_file);
        return -1;
    }

    fclose(stat_file);

    int matched = sscanf(line, "cpu %llu %llu %llu %llu %llu %llu %llu",
                          user, nice, system, idle, iowait, irq, softirq);

    return (matched == 7) ? 0 : -1;
}

static double measure_cpu_usage_percent(void)
{
    unsigned long long user1, nice1, system1, idle1, iowait1, irq1, softirq1;
    unsigned long long user2, nice2, system2, idle2, iowait2, irq2, softirq2;

    if (read_cpu_times(&user1, &nice1, &system1, &idle1, &iowait1, &irq1, &softirq1) != 0) {
        return 0.0;
    }
    sleep(1);
    if (read_cpu_times(&user2, &nice2, &system2, &idle2, &iowait2, &irq2, &softirq2) != 0) {
        return 0.0;
    }

    unsigned long long idle_delta = idle2 - idle1;
    unsigned long long total1 = user1 + nice1 + system1 + idle1 + iowait1 + irq1 + softirq1;
    unsigned long long total2 = user2 + nice2 + system2 + idle2 + iowait2 + irq2 + softirq2;
    unsigned long long total_delta = total2 - total1;

    if (total_delta == 0) {
        return 0.0;
    }

    return (1.0 - ((double) idle_delta / (double) total_delta)) * 100.0;
}

// ----------------- public API -----------------

int collect_system_info(struct system_info *out)
{
    memset(out, 0, sizeof(struct system_info));

    // RAM + uptime
    struct sysinfo info;
    if (sysinfo(&info) != 0) {
        return -1;
    }
    out->total_ram_bytes = (unsigned long long) info.totalram * info.mem_unit;
    out->free_ram_bytes = (unsigned long long) info.freeram * info.mem_unit;
    out->uptime_seconds = info.uptime;

    // Network interfaces
    struct ifaddrs *addrs = NULL;
    struct ifaddrs *current;

    if (getifaddrs(&addrs) != 0) {
        return -1;
    }

    for (current = addrs; current != NULL && out->interface_count < MAX_INTERFACES; current = current->ifa_next) {
        if (strcmp(current->ifa_name, "lo") == 0) {
            continue;
        }
        if (current->ifa_addr == NULL || current->ifa_addr->sa_family != AF_INET) {
            continue;
        }

        struct iface_info *slot = &out->interfaces[out->interface_count];

        strncpy(slot->name, current->ifa_name, sizeof(slot->name) - 1);

        struct sockaddr_in *addr_in = (struct sockaddr_in *) current->ifa_addr;
        inet_ntop(AF_INET, &(addr_in->sin_addr), slot->ip, INET_ADDRSTRLEN);

        strcpy(slot->netmask, "N/A");
        if (current->ifa_netmask != NULL && current->ifa_netmask->sa_family == AF_INET) {
            struct sockaddr_in *netmask_in = (struct sockaddr_in *) current->ifa_netmask;
            inet_ntop(AF_INET, &(netmask_in->sin_addr), slot->netmask, INET_ADDRSTRLEN);
        }

        get_iface_bytes(current->ifa_name, &slot->rx_bytes, &slot->tx_bytes);

        out->interface_count++;
    }

    freeifaddrs(addrs);

    // CPU usage (this blocks for ~1 second)
    out->cpu_usage_percent = measure_cpu_usage_percent();

    return 0;
}
