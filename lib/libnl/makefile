
NL_VER   = 3.2.21
URL      = http://www.infradead.org/~tgr/libnl/files/libnl-$(NL_VER).tar.gz
MAKEVARS = 
FINALTARGET = installed
AUTOMAKE_ROMFS = my_romfs

include $(ROOTDIR)/tools/automake.inc

my_romfs:
	cp -a build/libnl-$(NL_VER)-install/lib/*.so* $(ROMFSDIR)/lib/.

