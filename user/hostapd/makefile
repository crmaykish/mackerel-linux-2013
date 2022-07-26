
all: hostapd/.config
	$(MAKE) -C hostapd

hostapd/.config: hostapd/defconfig
	ln -s `pwd`/hostapd/defconfig `pwd`/hostapd/.config

romfs:
	$(ROMFSINST) -e CONFIG_USER_HOSTAPD_HOSTAPD hostapd/hostapd /bin/hostapd
	$(ROMFSINST) -e CONFIG_USER_HOSTAPD_HOSTAPD_CLI \
			hostapd/hostapd_cli /bin/hostapd_cli

clean:
	$(MAKE) -C hostapd clean
	rm hostapd/.config
