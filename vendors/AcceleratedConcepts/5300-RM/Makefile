#
# Makefile -- Build instructions for AcceleratedConcepts/5300-RM
#

ROMFSIMG  = $(IMAGEDIR)/rootfs.bin
UROMFSIMG = $(IMAGEDIR)/urootfs.bin
ZIMAGE    = $(IMAGEDIR)/zImage
IMAGE     = $(IMAGEDIR)/image.bin
UKERNEL   = $(IMAGEDIR)/ukernel.bin
IMAGESIZE = 7864320

ROMFS_DIRS = $(DEFAULT_ROMFS_DIRS)
ROMFS_DIRS += etc etc/config home proc sys tmp usr/var var

DEVICES += $(DEVICE_PTY_64) \
	ledman,c,126,0

FLASH_DEVICES = \
	boot,c,90,0 \
	bootenv,c,90,2 \
	config,c,90,4 \
	configblock,b,31,2 \
	image,c,90,6 \
	image1,c,90,8 \
	all,c,90,10

all:

clean: image.clean

romfs: romfs_dev romfs.dirs romfs.default romfs.rc romfs.default romfs.version
	$(ROMFSINST) -s /var/tmp/log /dev/log
	$(ROMFSINST) -s /var/run /run
	$(ROMFSINST) -s /etc/config/syslog.conf -e CONFIG_USER_SYSKLOGD /etc/syslog.conf
	$(ROMFSINST) /etc/default/start
	echo "Accelerated Concepts 5300-DC" > $(ROMFSDIR)/etc/issue
	echo "5300-DC" > $(ROMFSDIR)/etc/default/hostname

romfs.post::
	$(ROMFSINST) -R /share
	$(ROMFSINST) -R /etc/usb.ids.gz
	$(ROMFSINST) -R /bin/gettextize
	$(ROMFSINST) -R /bin/iconv
	$(ROMFSINST) -R /lib/pkgconfig
	$(ROMFSINST) -R /lib/gio
	$(ROMFSINST) -R /lib/dbus-1.0
	for i in $(ROMFSDIR)/lib/modules/*/*; do [ ! -f "$$i.bin" ] || rm -f "$$i"; done
	$(ROOTDIR)/tools/libclean.sh $(ROMFSDIR)

uimage.bin:
	cp $(ROOTDIR)/$(LINUXDIR)/arch/arm/boot/zImage $(ZIMAGE)
	cat $(ROOTDIR)/$(LINUXDIR)/arch/arm/boot/dts/imx50-ac5300rm.dtb >> $(ZIMAGE)
	mkimage -A arm -O linux -T kernel -C none -a 0x70060000 -e 0x70060000 -n "Linux-3.x" -d $(ZIMAGE) $(UKERNEL)
	mkimage -A arm -O linux -T ramdisk -C none -a 0x70800000 -n "ramdisk" -d $(ROMFSIMG) $(UROMFSIMG)
	SIZE=`wc -c < $(ROMFSIMG)` ; \
	[ "$$SIZE" -le 13238272 ] || { echo "ROOTFS Too big.";exit 1; }; \
	SIZE=`wc -c < $(UKERNEL)` ; \
	PAD=`expr 65536 - $$SIZE % 65536` ; \
	dd if=/dev/zero of=$(UKERNEL).pad bs=1 count=$$PAD
	cat $(UKERNEL) $(UKERNEL).pad $(ROMFSIMG) > $(IMAGE).old
	cat $(ROMFSIMG) $(UKERNEL)  > $(IMAGE)
	rm -f $(UKERNEL).pad

image: image.configs image.dir image.arm.zimage image.squashfs uimage.bin image.size image.tag image.copy

include $(ROOTDIR)/vendors/config/config.dev
include $(ROOTDIR)/vendors/AcceleratedConcepts/vendor.mak
