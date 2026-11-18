.DEFAULT_GOAL := help

ROUTER ?= root@192.168.1.1
OBJECT ?= openvpn.server1
CLIENT ?= client1

FILES := usr/bin/openvpn-monitor \
	etc/init.d/openvpn-monitor \
	etc/hotplug.d/openvpn/90-openvpn-monitor

export OBJECT CLIENT

.PHONY: help deploy restart objects clients disconnect

help:
	@printf '%s\n' \
		'make deploy                 Copy the service to the router, enable and start it' \
		'make restart                Restart the service' \
		'make objects                List the registered openvpn.* ubus objects' \
		'make clients                Call get_clients on OBJECT' \
		'make disconnect CLIENT=name Disconnect a client by common name' \
		'' \
		'Defaults: ROUTER=$(ROUTER) OBJECT=$(OBJECT)'

deploy:
	ssh "$(ROUTER)" "mkdir -p /etc/hotplug.d/openvpn"
	for f in $(FILES); do scp -O "$$f" "$(ROUTER):/$$f" || exit 1; done
	ssh "$(ROUTER)" "chmod +x $(addprefix /,$(FILES)) && \
		lua -e 'assert(loadfile(\"/usr/bin/openvpn-monitor\"))' && \
		/etc/init.d/openvpn-monitor enable && \
		/etc/init.d/openvpn-monitor restart"

restart:
	ssh "$(ROUTER)" "/etc/init.d/openvpn-monitor restart"

objects:
	ssh "$(ROUTER)" "ubus list 'openvpn.*'"

clients:
	@case "$$OBJECT" in ''|*[!A-Za-z0-9._-]*) echo 'Invalid OBJECT value' >&2; exit 2;; esac
	ssh "$(ROUTER)" "ubus call '$$OBJECT' get_clients"

disconnect:
	@case "$$OBJECT" in ''|*[!A-Za-z0-9._-]*) echo 'Invalid OBJECT value' >&2; exit 2;; esac
	@case "$$CLIENT" in ''|*[!A-Za-z0-9._@-]*) echo 'Invalid CLIENT value' >&2; exit 2;; esac
	ssh "$(ROUTER)" "ubus call '$$OBJECT' disconnect '{\"name\":\"$$CLIENT\"}'"
