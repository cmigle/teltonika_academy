# OpenVPN monitor

A Lua service for Teltonika RutOS / OpenWrt routers that shows who is connected to each OpenVPN server and can disconnect them.

For every enabled OpenVPN server in `/etc/config/openvpn`, it registers a ubus object named `openvpn.<server name>`. Each server gets its own object, so several servers can run at the same time. It was developed and tested on a Teltonika RUTX10 with multiple servers and clients.

```
ubus call openvpn.<server> ──▶ openvpn-monitor ──TCP──▶ OpenVPN management interface
                                     │                    "status 2" / "kill <cn>"
                                     └── reads /etc/config/openvpn through UCI
```

## ubus API

| Method | Parameters | Description |
|--------|------------|-------------|
| `get_clients` | none | Lists connected clients |
| `disconnect` | `name` (string) | Disconnects a client by certificate common name |

```sh
ubus list 'openvpn.*'
ubus call openvpn.server1 get_clients
ubus call openvpn.server1 disconnect '{"name":"client1"}'
```

Example `get_clients` reply:

```json
{
  "server": "server1",
  "client_count": 1,
  "users": [
    {
      "common_name": "client1",
      "real_address": "192.168.1.100:54321",
      "virtual_address": "10.8.0.6",
      "virtual_ipv6": "",
      "bytes_received": 18734,
      "bytes_sent": 20511,
      "connected_since": "2026-10-10 14:02:11",
      "connected_since_t": 1791630131,
      "client_id": 3,
      "peer_id": 0,
      "cipher": "AES-256-GCM"
    }
  ]
}
```

`disconnect` checks that the client exists before sending `kill`, and returns `success` together with either a message or an error.

## Files

The directory mirrors the router's filesystem:

| File | Purpose |
|------|---------|
| [`usr/bin/openvpn-monitor`](usr/bin/openvpn-monitor) | The service: UCI lookup, management interface client, status parser, ubus objects |
| [`etc/init.d/openvpn-monitor`](etc/init.d/openvpn-monitor) | procd init script |
| [`etc/hotplug.d/openvpn/90-openvpn-monitor`](etc/hotplug.d/openvpn/90-openvpn-monitor) | Re-checks the service when an OpenVPN tunnel goes up or down |
| [`Makefile`](Makefile) | Deploys to a router over SSH and wraps the ubus calls |

## When it runs

The init script starts the service only if at least one OpenVPN server is:

1. enabled (`enable='1'`),
2. configured with a management interface, and
3. actually running (its `openvpn` process exists).

Otherwise it logs a message and exits. This check runs again at boot, whenever `/etc/config/openvpn` changes (procd reload trigger), and on every tunnel up/down event (hotplug). A server with broken configuration is logged and skipped, and the other servers still register.

## Server requirements

- **TLS mode.** In static key mode only one peer can connect and it has no certificate common name, so there is nothing to list or kill.
- **Management interface**, e.g. `management 127.0.0.1 7505` in the WebUI's *Extra options* field.

Clients are read with the management command `status 2`, which always uses the status-version 2 format whatever the server's configuration says. If a server has no management interface, the service falls back to reading `/var/run/openvpn.<instance>.status`.

## Install

```sh
make deploy ROUTER=root@192.168.1.1   # copies the files, enables and starts the service
make clients OBJECT=openvpn.server1
make disconnect OBJECT=openvpn.server1 CLIENT=client1
```

No compiling is needed: the service is plain Lua using the `ubus`, `uci`, `uloop` and `nixio` modules that ship with OpenWrt.

## Things learned on real hardware

- The router's original server ran in static key mode, so clients couldn't be told apart. The fix was a PKI built with easy-rsa (a CA, a server certificate and two client certificates) and a switch to TLS mode.
- RutOS's config generator ignores some standard UCI options, such as `status` and `status_version`, and keeps custom directives in a single `extra` option. That is why the service reads the management address from `extra` and queries the management interface instead of relying on the status file.
- In the status-version 2 format, field 5 is the virtual IPv6 address. Indexing by position read "bytes received" from the wrong column, so the parser now maps every field explicitly.
- Testing with several servers turned up a crash. Multi-line *Extra options* are stored by UCI as a list, not a string, and one bad server stopped all the others from registering. Each server is now registered inside `pcall`.
