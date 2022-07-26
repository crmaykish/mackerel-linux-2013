#
#	Accelecon Factory
#

ZIMAGE    = $(IMAGEDIR)/bzImage
ROMFSIMG  = $(IMAGEDIR)/initrd
ROMFSIMGZ = $(IMAGEDIR)/initrd.gz

ISOFILE   = $(IMAGEDIR)/factory.iso
ISODIR    = $(ROOTDIR)/iso

ROMFS_DIRS = $(DEFAULT_ROMFS_DIRS)

# FLASH_DEVICES =

all:

clean: image.clean

romfs: romfs_dev romfs.dirs romfs.symlinks romfs.default romfs.version
	$(ROMFSINST) /etc/default/passwd
	$(ROMFSINST) /etc/default/shadow
	$(ROMFSINST) /etc/default/group
	$(ROMFSINST) /etc/rc
	$(ROMFSINST) /etc/default/start
	# echo "ttyS0:unknown:/sbin/getty 115200 -" >> $(ROMFSDIR)/etc/inittab
	# echo "tty1:unknown:/sbin/getty 9600 -" >> $(ROMFSDIR)/etc/inittab
	# echo "tty2:unknown:/sbin/getty 9600 -" >> $(ROMFSDIR)/etc/inittab

romfs.post::
	# nothing for now

image: image.configs image.dir image.i386.zimage image.squashfs
	gzip -9 -f $(ROMFSIMG)
	mkdir -p $(ISODIR)/boot/isolinux
	cp isolinux.bin $(ISODIR)/boot/isolinux/
	cp boot.msg $(ISODIR)/boot/isolinux/
	cp isolinux.cfg $(ISODIR)/boot/isolinux/
	cp $(ZIMAGE) $(ISODIR)/boot/isolinux/
	cp $(ROMFSIMGZ) $(ISODIR)/boot/isolinux/initrd.gz
	mkisofs -J -o $(ISOFILE) -b boot/isolinux/isolinux.bin \
		-c boot/isolinux/boot.cat -no-emul-boot \
		-boot-load-size 4 -boot-info-table $(ISODIR)

include $(ROOTDIR)/vendors/config/config.dev
include ../vendor.mak
