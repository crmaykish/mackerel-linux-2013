
URL = https://www.kernel.org/pub/linux/utils/kernel/hotplug/udev-182.tar.bz2

CONFOPTS += --disable-introspection
ifndef CONFIG_LIB_GLIB
CONFOPTS += --disable-gudev
else
CONFOPTS += --enable-gudev
endif
CONFOPTS += --disable-manpages --disable-gtk-doc-html
CONFOPTS += --enable-fast-install --disable-dependency-tracking
CONFOPTS += --disable-keymap
CONFOPTS += --disable-mtd_probe
CONFOPTS += --with-usb-ids-path=/etc/usb.ids
CONFOPTS += --with-pci-ids-path=/etc/pci.ids
CONFOPTS += --sysconfdir=/etc
# CONFOPTS += --enable-debug

CONFVARS = BLKID_CFLAGS=" " BLKID_LIBS="-lblkid" LIBS=-lrt
MAKEVARS = 

ROMFS    = extra_romfs

include $(ROOTDIR)/tools/automake.inc

extra_romfs:
	$(ROMFSINST) -d /etc/init.d/S10udev

