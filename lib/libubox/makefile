
URL      = git://nbd.name/luci2/libubox.git
METHOD   = git
AUTOCONF = cmake CMakeLists.txt -DBUILD_LUA=0
AUTOCONF += -DCMAKE_INSTALL_PREFIX=/ -DCMAKE_SYSTEM_PREFIX_PATH=$(STAGEDIR)
libubox_ROMFS = romfs_fixup

include $(ROOTDIR)/tools/automake.inc

romfs_fixup:
	mkdir -p $(ROMFSDIR)/usr/share
	cp -afL $(ROMFSDIR)/share/libubox $(ROMFSDIR)/usr/share/.
	rm -rf $(ROMFSDIR)/share/libubox 

