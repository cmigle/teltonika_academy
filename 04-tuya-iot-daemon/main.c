#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>
#include "args.h"
#include "daemonize.h"
#include "sysinfo.h"
#include "tuya_client.h"

#define REPORT_INTERVAL_SECONDS 30
#define LOOP_SLEEP_USECONDS 200000  // 200ms - how often tuya_client_loop() is polled

int main(int argc, char **argv)
{
    struct arguments args;
    parse_arguments(argc, argv, &args);

    if (args.daemon_mode) {
        daemonize();
    }

    openlog("tuya_daemon", LOG_PID, LOG_DAEMON);

    syslog(LOG_INFO, "tuya_daemon started (device_id=%s, product_id=%s, daemon_mode=%s)",
           args.device_id, args.product_id, args.daemon_mode ? "yes" : "no");

    if (tuya_client_start(args.product_id, args.device_id, args.device_secret) != 0) {
        syslog(LOG_ERR, "failed to start Tuya client, exiting");
        closelog();
        return 1;
    }

    // FIX: track real elapsed wall-clock time with time(), not a loop-iteration
    // counter - each loop iteration only sleeps 200ms, so counting iterations
    // and comparing against a value in SECONDS was a unit mismatch.
    time_t last_report_time = 0; // 0 forces an immediate report on first iteration

    while (1) {
        // tuya_mqtt_loop() must be called frequently and must never be
        // blocked by our own sleep()s, or keepalive/incoming Action messages
        // would be delayed or missed entirely.
        tuya_client_loop();

        time_t now = time(NULL);
        if (now - last_report_time >= REPORT_INTERVAL_SECONDS) {
            struct system_info info;

            if (collect_system_info(&info) == 0) {
                // collect_system_info() blocks ~1s internally (CPU measurement)
                syslog(LOG_INFO, "RAM total=%llu MB free=%llu MB, uptime=%ld s, CPU=%.2f%%",
                       info.total_ram_bytes / 1024 / 1024,
                       info.free_ram_bytes / 1024 / 1024,
                       info.uptime_seconds,
                       info.cpu_usage_percent);

                if (tuya_client_report_system_info(&info) != 0) {
                    syslog(LOG_ERR, "failed to report system info to Tuya cloud");
                }
            } else {
                syslog(LOG_ERR, "failed to collect system info");
            }

            last_report_time = time(NULL);
        }

        usleep(LOOP_SLEEP_USECONDS);
    }

    closelog();
    return 0;
}
