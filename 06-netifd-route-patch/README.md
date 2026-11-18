# netifd patch: gateway for static routes without one

A fix for [OpenWrt issue #10791](https://github.com/openwrt/openwrt/issues/10791) in **netifd**, OpenWrt's network configuration daemon.

Patch: [`0001-interface-ip-use-interface-gateway-for-static-routes.patch`](0001-interface-ip-use-interface-gateway-for-static-routes.patch)

## The problem

LuCI's help text for a static route's *Gateway* field says:

> If omitted, the gateway from the parent interface is taken if any, otherwise creates a link scope route.

netifd doesn't do that. A route without a gateway is always installed as a link-scope route:

```
# actual
8.8.8.8 dev eth1 proto static scope link metric 1

# expected
8.8.8.8 via 192.168.222.1 dev eth1 proto static metric 1
```

A link-scope route makes the kernel treat the target as directly attached, so it sends ARP requests for it on the local segment and the traffic never reaches it. On DHCP, NCM or QMI uplinks this makes the route useless. This matters most where the provider hands out a different gateway each time, which is exactly when you'd want to leave the field empty.

## Root cause

The behaviour was documented but never implemented. `git log -S nexthop -- interface-ip.c` shows no commit that ever added gateway inheritance.

| Where | What happens |
|-------|--------------|
| `config.c`: `config_parse_route()` | UCI `route` sections are added to the interface's static `config_ip`, not `proto_ip` where the DHCP/LTE default route and its gateway live |
| `interface-ip.c`: `interface_ip_add_route()` | The route struct starts zeroed, and `nexthop` is only filled when `option gateway` is set. Nothing looks at the protocol routes |
| `system-linux.c`: `system_rt()` | No nexthop means `RT_SCOPE_LINK` and no `RTA_GATEWAY` attribute is sent to the kernel |

## The fix

`interface_add_config_route()` is used in place of `system_add_route()` for static routes. When a route has no gateway, isn't `onlink`, isn't a typed route (blackhole, unreachable, ...) and its target is outside the interface's own subnet, it copies the nexthop of the best-matching protocol route on the same interface into **a copy** of the route that goes to the kernel.

The stored route keeps its empty nexthop. That matters for two reasons:

- netifd identifies routes by comparing their fields, so leaving the stored route unchanged keeps add, update and delete working exactly as before.
- Because the empty nexthop is still visible, `interface_refresh_config_routes()` can find these routes and reinstall them when the protocol routes change (for example after a DHCP renewal with a new gateway) and when the interface comes back up.

The first version of the patch added a new `DEVROUTE_GATEWAY` flag instead. It was rejected in review because `route_cmp()` compares flags, so a new flag would have changed how netifd matches routes. The second version adds no new state.

## Testing

netifd was built from source and run in its test mode (`DUMMY_MODE`), comparing the routes it produced before and after the patch:

1. Route without a gateway: now installed `via` the interface's gateway
2. Gateway changes: the route follows the new gateway
3. Interface down and up: the route is removed and reinstalled correctly
4. Route with an explicit gateway: unchanged
5. Route to a target inside the interface's own subnet: still link-scope

## Applying

The patch was made with `git format-patch` and follows OpenWrt's conventions: tab indentation, `/* */` comments only, and a `Signed-off-by` line.

```sh
git clone https://git.openwrt.org/project/netifd.git
cd netifd
git am /path/to/0001-interface-ip-use-interface-gateway-for-static-routes.patch
```
