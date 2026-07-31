#ifndef TUYA_CLIENT_H
#define TUYA_CLIENT_H

#include "sysinfo.h"

// Initializes and connects a Tuya MQTT client using the given credentials.
// Blocks until either connected or a fatal error occurs (mirrors the
// SDK's own examples). Returns 0 on success, non-zero on failure.
int tuya_client_start(const char *product_id, const char *device_id, const char *device_secret);

// Must be called repeatedly (frequently - several times per second) from
// the daemon's main loop. Handles keepalive and incoming messages
// (including Action invocations). Non-blocking.
void tuya_client_loop(void);

// Reports a snapshot of system info to the cloud as a batch of properties.
// Safe to call periodically (e.g. every 30s) from the main loop.
int tuya_client_report_system_info(const struct system_info *info);

#endif
