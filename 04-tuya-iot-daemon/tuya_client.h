#ifndef TUYA_CLIENT_H
#define TUYA_CLIENT_H

int tuya_client_start(const char *product_id,
                      const char *device_id,
                      const char *device_secret);

void tuya_client_loop(void);

int tuya_client_stop(void);

#endif