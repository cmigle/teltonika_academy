#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <uci.h>
#include <libubus.h>
#include <libubox/blobmsg.h>
#include "sysinfo.h"

/* ------------------------------------------------------------------ */
/* CPU — no ubus equivalent, keep /proc/stat measurement              */
/* ------------------------------------------------------------------ */

static int read_cpu_times(unsigned long long *user, unsigned long long *nice,
                          unsigned long long *system, unsigned long long *idle,
                          unsigned long long *iowait, unsigned long long *irq,
                          unsigned long long *softirq)
{
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return -1;
    char line[256];
    if (!fgets(line, sizeof(line), f)) { fclose(f); return -1; }
    fclose(f);
    int matched = sscanf(line, "cpu %llu %llu %llu %llu %llu %llu %llu",
                         user, nice, system, idle, iowait, irq, softirq);
    return (matched == 7) ? 0 : -1;
}

static double measure_cpu_usage_percent(void)
{
    unsigned long long u1,n1,s1,i1,w1,r1,q1;
    unsigned long long u2,n2,s2,i2,w2,r2,q2;
    if (read_cpu_times(&u1,&n1,&s1,&i1,&w1,&r1,&q1) != 0) return 0.0;
    sleep(1);
    if (read_cpu_times(&u2,&n2,&s2,&i2,&w2,&r2,&q2) != 0) return 0.0;
    unsigned long long idle_delta  = i2 - i1;
    unsigned long long total_delta = (u2+n2+s2+i2+w2+r2+q2)
                                   - (u1+n1+s1+i1+w1+r1+q1);
    if (total_delta == 0) return 0.0;
    return (1.0 - (double)idle_delta / (double)total_delta) * 100.0;
}


/* ------------------------------------------------------------------ */
/* ubus callback — system info (RAM + uptime)                          */
/* ------------------------------------------------------------------ */

struct system_cb_data {
    unsigned long long total_ram;
    unsigned long long free_ram;
    long               uptime;
};

static void system_info_cb(struct ubus_request *req, int type, struct blob_attr *msg)
{
    struct system_cb_data *d = req->priv;

    static const struct blobmsg_policy policy[] = {
        [0] = { "uptime", BLOBMSG_TYPE_INT32 },
        [1] = { "memory", BLOBMSG_TYPE_TABLE },
    };
    struct blob_attr *tb[2];
    blobmsg_parse(policy, 2, tb, blob_data(msg), blob_len(msg));

    if (tb[0])
        d->uptime = (long)blobmsg_get_u32(tb[0]);

    if (tb[1]) {
    static const struct blobmsg_policy mem_policy[] = {
        [0] = { "total", BLOBMSG_TYPE_INT64 },
        [1] = { "free",  BLOBMSG_TYPE_INT64 },
    };
    struct blob_attr *mem[2];
    blobmsg_parse(mem_policy, 2, mem, blobmsg_data(tb[1]), blobmsg_data_len(tb[1]));
    if (mem[0]) 
    {
        d->total_ram = blobmsg_get_u64(mem[0]);
    }
    if (mem[1]) 
    {
        d->free_ram  = blobmsg_get_u64(mem[1]);
    }
}

}

/* ------------------------------------------------------------------ */
/* ubus callback — network.device status (rx/tx bytes per device)      */
/* ------------------------------------------------------------------ */

struct dev_stat {
    char               name[64];
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
};

struct device_cb_data {
    struct dev_stat devs[MAX_INTERFACES];
    int             count;
};

static void network_device_cb(struct ubus_request *req, int type, struct blob_attr *msg)
{
    struct device_cb_data *d = req->priv;
    struct blob_attr *cur;
    int rem;

    blob_for_each_attr(cur, msg, rem) {
        if (d->count >= MAX_INTERFACES) break;

        const char *devname = blobmsg_name(cur);
        if (strcmp(devname, "lo") == 0) continue;

        static const struct blobmsg_policy stat_policy[] = {
            [0] = { "statistics", BLOBMSG_TYPE_TABLE },
        };
        struct blob_attr *stat_tb[1];
        blobmsg_parse(stat_policy, 1, stat_tb,
                      blobmsg_data(cur), blobmsg_data_len(cur));
        if (!stat_tb[0]) continue;

        static const struct blobmsg_policy bytes_policy[] = {
            [0] = { "rx_bytes", BLOBMSG_TYPE_INT64 },
            [1] = { "tx_bytes", BLOBMSG_TYPE_INT64 },
        };
        struct blob_attr *bytes[2];
        blobmsg_parse(bytes_policy, 2, bytes,
                      blobmsg_data(stat_tb[0]), blobmsg_data_len(stat_tb[0]));

        strncpy(d->devs[d->count].name, devname,
                sizeof(d->devs[d->count].name) - 1);
        if (bytes[0]) d->devs[d->count].rx_bytes = blobmsg_get_u64(bytes[0]);
        if (bytes[1]) d->devs[d->count].tx_bytes = blobmsg_get_u64(bytes[1]);
        d->count++;
    }
}

/* ------------------------------------------------------------------ */
/* ubus callback — network.interface status (IPs mapped to devices)   */
/* ------------------------------------------------------------------ */

struct iface_status {
    char device [64];
    char ip     [INET_ADDRSTRLEN];
    char netmask[INET_ADDRSTRLEN];
    int  valid;
};

static void iface_status_cb(struct ubus_request *req, int type, struct blob_attr *msg)
{
    struct iface_status *d = req->priv;

    static const struct blobmsg_policy policy[] = {
        [0] = { "device",       BLOBMSG_TYPE_STRING },
        [1] = { "ipv4-address", BLOBMSG_TYPE_ARRAY  },
    };
    struct blob_attr *tb[2];
    blobmsg_parse(policy, 2, tb, blob_data(msg), blob_len(msg));

    if (!tb[0] || !tb[1]) {
        syslog(LOG_WARNING, "iface_status_cb: missing device or ipv4-address");
        return;
    }

    strncpy(d->device, blobmsg_get_string(tb[0]), sizeof(d->device) - 1);

    syslog(LOG_INFO, "iface_status_cb: ipv4-address blob size=%d",
           (int)blobmsg_data_len(tb[1]));

    struct blob_attr *cur;
    int rem;
    int loop_count = 0;
    __blob_for_each_attr(cur, blobmsg_data(tb[1]), rem) {
        loop_count++;
        syslog(LOG_INFO, "iface iteration #%d type=%d",
               loop_count, blobmsg_type(cur));

        static const struct blobmsg_policy addr_policy[] = {
            [0] = { "address", BLOBMSG_TYPE_STRING },
            [1] = { "mask",    BLOBMSG_TYPE_INT32  },
        };
        struct blob_attr *addr[2];
        blobmsg_parse(addr_policy, 2, addr,
                      blobmsg_data(cur), blobmsg_data_len(cur));

        if (!addr[0]) {
            syslog(LOG_WARNING, "iface iteration #%d: address is NULL", loop_count);
            continue;
        }

        strncpy(d->ip, blobmsg_get_string(addr[0]), INET_ADDRSTRLEN - 1);

        if (addr[1]) {
            int prefix = (int)blobmsg_get_u32(addr[1]);
            unsigned int mask = prefix ? (~0u << (32 - prefix)) : 0;
            snprintf(d->netmask, INET_ADDRSTRLEN,
                     "%u.%u.%u.%u",
                     (mask >> 24) & 0xff, (mask >> 16) & 0xff,
                     (mask >>  8) & 0xff,  mask         & 0xff);
        } else {
            strncpy(d->netmask, "N/A", INET_ADDRSTRLEN);
        }

        d->valid = 1;
        break;
    }
    syslog(LOG_INFO, "iface loop total iterations: %d", loop_count);
}


/* ------------------------------------------------------------------ */
/* collect interface names from UCI I /etc/config/network             */
/* ------------------------------------------------------------------ */

static int collect_interface_names(char iface_names[][64], int max_count)
{
    struct uci_context *ctx = uci_alloc_context();
    if (!ctx) {
        syslog(LOG_ERR, "sysinfo: failed to allocate UCI context");
        return 0;
    }

    struct uci_package *pkg = NULL;
    if (uci_load(ctx, "network", &pkg) != UCI_OK) {
        syslog(LOG_ERR, "sysinfo: failed to load UCI network config");
        uci_free_context(ctx);
        return 0;
    }

    int count = 0;
    struct uci_element *e;
    uci_foreach_element(&pkg->sections, e) {
        struct uci_section *s = uci_to_section(e);

        if (strcmp(s->type, "interface") != 0) continue;
        if (strcmp(s->e.name, "loopback") == 0) continue;
        if (count >= max_count) break;

        strncpy(iface_names[count], s->e.name, 63);
        iface_names[count][63] = '\0';
        count++;
    }

    uci_free_context(ctx);
    return count;
}



/* ------------------------------------------------------------------ */
/* public API                                                         */
/* ------------------------------------------------------------------ */

int collect_system_info(struct system_info *out)
{
    memset(out, 0, sizeof(*out));

    struct ubus_context *ctx = ubus_connect("/tmp/run/ubus/ubus.sock");
    if (!ctx) {
        syslog(LOG_ERR, "sysinfo: failed to connect to ubus");
        return -1;
    }

    uint32_t id;

    /* RAM + uptime */
    struct system_cb_data sys_data = {0};
    if (ubus_lookup_id(ctx, "system", &id) == 0) {
        ubus_invoke(ctx, id, "info", NULL,
                    system_info_cb, &sys_data, 3000);
        out->total_ram_bytes = sys_data.total_ram;
        out->free_ram_bytes  = sys_data.free_ram;
        out->uptime_seconds  = sys_data.uptime;
    } else {
        syslog(LOG_WARNING, "sysinfo: ubus 'system' not found");
    }

    /* rx/tx bytes from network.device */
    struct device_cb_data dev_data = {0};
    if (ubus_lookup_id(ctx, "network.device", &id) == 0) {
        ubus_invoke(ctx, id, "status", NULL,
                    network_device_cb, &dev_data, 3000);
    } else {
        syslog(LOG_WARNING, "sysinfo: ubus 'network.device' not found");
    }

    /* interface names from UCI */
    char iface_names[MAX_INTERFACES][64];
    int iface_count = collect_interface_names(iface_names, MAX_INTERFACES);

    syslog(LOG_INFO, "collected %d interface names from UCI", iface_count);

    for (int i = 0; i < iface_count && out->interface_count < MAX_INTERFACES; i++) {

        syslog(LOG_INFO, "processing interface: %s", iface_names[i]);

        char ubus_obj[128];
        snprintf(ubus_obj, sizeof(ubus_obj), "network.interface.%s", iface_names[i]);

        struct iface_status status = {0};
        if (ubus_lookup_id(ctx, ubus_obj, &id) != 0) {
            syslog(LOG_WARNING,
                "sysinfo: ubus object '%s' not found", ubus_obj);
            continue;
        }
        ubus_invoke(ctx, id, "status", NULL, iface_status_cb, &status, 3000);

        syslog(LOG_INFO, "interface %s: valid=%d ip=%s device=%s", iface_names[i], status.valid, status.ip, status.device);

            if (!status.valid) continue;

            struct iface_info *slot = &out->interfaces[out->interface_count];

            strncpy(slot->name,    iface_names[i],  sizeof(slot->name) - 1);
            strncpy(slot->ip,      status.ip,        INET_ADDRSTRLEN - 1);
            strncpy(slot->netmask, status.netmask,   INET_ADDRSTRLEN - 1);

            slot->rx_bytes = 0;
            slot->tx_bytes = 0;
            for (int j = 0; j < dev_data.count; j++) {
                if (strcmp(dev_data.devs[j].name, status.device) == 0) {
                    slot->rx_bytes = dev_data.devs[j].rx_bytes;
                    slot->tx_bytes = dev_data.devs[j].tx_bytes;
                    break;
                }
            }

           out->interface_count++;
    }

    ubus_free(ctx);

    /* CPU — no ubus equivalent */
    out->cpu_usage_percent = measure_cpu_usage_percent();

    return 0;
}
