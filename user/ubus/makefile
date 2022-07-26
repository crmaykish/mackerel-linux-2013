
URL      = git://nbd.name/luci2/ubus.git
METHOD   = git
AUTOCONF = cmake CMakeLists.txt
AUTOCONF += -DBUILD_LUA=0 -DBUILD_EXAMPLES=0 -DENABLE_SYSTEMD=0
AUTOCONF += -DCMAKE_INSTALL_PREFIX=/ -DCMAKE_SYSTEM_PREFIX_PATH=$(STAGEDIR)

include $(ROOTDIR)/tools/automake.inc

