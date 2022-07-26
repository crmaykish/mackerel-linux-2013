#
#	Makefile -- Build instructions for AcceleratedConcepts/5300-DC
#

ROMFSIMG  = $(IMAGEDIR)/rootfs.bin
UROMFSIMG = $(IMAGEDIR)/urootfs.bin
ZIMAGE    = $(IMAGEDIR)/zImage
IMAGE     = $(IMAGEDIR)/image.bin
KERNEL    = $(IMAGEDIR)/kernel
UKERNEL   = $(IMAGEDIR)/ukernel.bin
IMAGESIZE = 7864320

ROMFS_DIRS = $(DEFAULT_ROMFS_DIRS)
ROMFS_DIRS += etc etc/config home proc sys tmp usr/var var

DEVICES += $(DEVICE_PTY_64) \
	slic0,c,25,0 \
	slic1,c,25,1

FLASH_DEVICES = \
	boot,c,90,0 \
	bootenv,c,90,2 \
	config,c,90,4 \
	configblock,b,31,2 \
	kernel,c,90,6 \
	rootfs,c,90,8 \
	image,c,90,10 \
	kernel1,c,90,12 \
	rootfs1,c,90,14 \
	image1,c,90,16 \
	all,c,90,18

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
	mkimage -A arm -O linux -T kernel -C none -a 0x80060000 -e 0x80060000 -n "Linux-3.x" -d $(ZIMAGE) $(UKERNEL)
	mkimage -A arm -O linux -T ramdisk -C none -a 0x80800000 -n "ramdisk" -d $(ROMFSIMG) $(UROMFSIMG)
	SIZE=`cat $(ROMFSIMG) | wc -c` ; \
	[ "$$SIZE" -le 13238272 ] || { echo "ROOTFS Too big.";exit 1; }; \
	SIZE=`cat $(UKERNEL) | wc -c` ; \
	PAD=`expr 65536 - $$SIZE % 65536` ; \
	dd if=/dev/zero of=$(UKERNEL).pad bs=1 count=$$PAD
	cat $(UKERNEL) $(UKERNEL).pad $(ROMFSIMG) > $(IMAGE)
	rm -f $(UKERNEL).pad

image: image.configs image.dir image.arm.zimage image.squashfs uimage.bin image.size image.tag image.copy

include $(ROOTDIR)/vendors/config/config.dev
include $(ROOTDIR)/vendors/AcceleratedConcepts/vendor.mak
