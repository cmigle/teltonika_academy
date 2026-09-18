#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>
#include "args.h"
#include "daemonize.h"
#include "tuya_client.h"

#define LOOP_SLEEP_USECONDS 200000  // 200ms - how often tuya_client_loop() is polled

static volatile sig_atomic_t g_should_exit = 0;

static void handle_shutdown_signal(int signum)
{
    (void) signum;
    g_should_exit = 1;
}

int main(int argc, char **argv)
{
    struct sigaction action;
    struct arguments args;

    memset(&action, 0, sizeof(action));
    action.sa_handler = handle_shutdown_signal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGINT, &action, NULL) != 0 || sigaction(SIGTERM, &action, NULL) != 0) {
        syslog(LOG_ERR, "failed to install signal handlers");
        return 1;
    }

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

    while (!g_should_exit) {
        // tuya_mqtt_loop() must be called frequently and must never be blocked
        tuya_client_loop();

        usleep(LOOP_SLEEP_USECONDS);
    }

    tuya_client_stop();
    closelog();
    return 0;
}
