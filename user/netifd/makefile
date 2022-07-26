
URL      = git://nbd.name/luci2/netifd.git
METHOD   = git
AUTOCONF = cmake CMakeLists.txt
AUTOCONF += -DCMAKE_INSTALL_PREFIX=/ -DCMAKE_SYSTEM_PREFIX_PATH=$(STAGEDIR)
netifd_ROMFS = my_romfs

include $(ROOTDIR)/tools/automake.inc

my_romfs:
	mkdir -p $(ROMFSDIR)/lib/netifd
	$(ROMFSINST) build/netifd/dummy/netifd-proto.sh /lib/netifd/netifd-proto.sh

# $(ROMFSINST) build/netifd/dummy/proto/ppp.sh /lib/netifd/proto/ppp.sh
# $(ROMFSINST) build/netifd/dummy/proto/pptp.sh /lib/netifd/proto/pptp.sh

