# Tuya IoT daemon

A C daemon that connects a Linux or OpenWrt device to the [Tuya IoT cloud](https://developer.tuya.com/) over MQTT (TLS, port 8883) using the [Tuya IoT Core SDK](https://github.com/tuya/tuya-iot-core-sdk). It receives commands ("actions") from the cloud, carries them out through a local [ubus](https://openwrt.org/docs/techref/ubus) service and publishes the result back to Tuya.

This is the last version of a program that grew over three academy assignments:

1. **Desktop Linux daemon.** Connects to Tuya and reports RAM, CPU load, uptime and network interfaces every 30 seconds.
2. **RutOS port.** Cross-compiled with the RutOS SDK for a Teltonika RUTX10 router, packaged as an `.ipk` alongside a separately packaged Tuya SDK, and started by procd. System data now comes from ubus and UCI instead of `/proc`.
3. **Cloud-to-device bridge.** Tuya actions are forwarded to a ubus service that drives ESP microcontrollers connected to the router over USB.

The [branch history](https://github.com/cmigle/teltonika_academy/commits/tuya-iot-daemon) has both the original desktop version (July) and this one.

## How it works

```
Tuya cloud ──MQTT/TLS──▶ tuya_daemon ──ubus──▶ esp service ──serial──▶ ESP board
           ◀─execute_response─┘
```

| Action code | Input parameters | ubus call |
|-------------|------------------|-----------|
| `list_esp_devices` | none | `esp devices` |
| `set_esp_pin` | `port` (string), `pin` (int), `state` (bool) | `esp on` / `esp off` |
| `get_esp_sensor` | `port`, `pin`, `model`, `sensor` | `esp get` |

The ubus reply goes back to Tuya as JSON on `tylink/<device_id>/thing/action/execute_response`. Unknown actions, malformed JSON and missing parameters are answered with an error code instead of being dropped.

## Source files

| File | Responsibility |
|------|----------------|
| [`main.c`](main.c) | Signal handling and the main loop. Polls the MQTT client every 200 ms and shuts down cleanly on `SIGINT`/`SIGTERM` |
| [`args.c`](args.c) | Command-line parsing (`-p`, `-d`, `-s`, `--daemon`) |
| [`daemonize.c`](daemonize.c) | Detaches from the terminal: `fork`, `setsid`, `chdir("/")`, `umask(0)`, stdio to `/dev/null` |
| [`tuya_client.c`](tuya_client.c) | MQTT setup, subscriptions, action parsing and dispatch, ubus calls, responses |
| [`sysinfo.c`](sysinfo.c) | Collects RAM, uptime, CPU load and per-interface addresses and traffic from ubus (`system`, `network.device`, `network.interface.*`) and UCI |
| [`tuya_cacert.h`](tuya_cacert.h) | Root CA certificate used to verify the Tuya MQTT broker |

All logging goes to syslog under the `tuya_daemon` tag.

## Build

Requires the Tuya IoT Core SDK and the OpenWrt libraries `libubus`, `libubox`, `libblobmsg_json` and `libuci`.

```sh
git clone https://github.com/tuya/tuya-iot-core-sdk ../tuya-iot-core-sdk
cmake -S ../tuya-iot-core-sdk -B ../tuya-iot-core-sdk/build && make -C ../tuya-iot-core-sdk/build
make SDK_DIR=../tuya-iot-core-sdk
```

## Run

Device credentials come from the device page on the Tuya IoT platform.

```sh
./tuya_daemon --product-id <PRODUCT_ID> --device-id <DEVICE_ID> --device-secret <DEVICE_SECRET>
./tuya_daemon -p <PRODUCT_ID> -d <DEVICE_ID> -s <DEVICE_SECRET> --daemon   # detach

logread -f | grep tuya_daemon     # OpenWrt
journalctl -t tuya_daemon -f      # systemd
```

## Problems solved along the way

- **Successful reports logged as errors.** `tuyalink_thing_property_report()` returns a positive message ID on success, not `0`. The check is now `ret < 0`.
- **No `argp` on the router.** RutOS uses musl libc, which doesn't ship `argp`, so arguments are parsed by hand.
- **Memory and traffic counters reading as zero.** ubus returns them as 64-bit integers and they were being read with `blobmsg_get_u32()`. Switched to `blobmsg_get_u64()`.
- **Cloud connection dropping a few seconds after connecting.** Buffer sizes, cloud-side throttling, the regional endpoint, DNS and the system clock were each ruled out in turn. The cause was a data-model request in `on_connected`: its response overflows the SDK's fixed receive buffer and the broker closes the connection. Removing the request made the connection stable.
- **Tracing cloud-to-device delivery.** The SDK only subscribes to one topic and relies on Tuya forwarding the rest on the server side. To see whether cloud commands reach the device, the daemon subscribes to `tylink/<device_id>/#` itself and sends a time-request probe on connect. The reply, or its absence, shows up in the log.
