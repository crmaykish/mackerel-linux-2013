#############################################################################
#
#	Makefile -- Build instructions for SnapGear/SE2910-CF board
#
#############################################################################

include $(LINUX_CONFIG)
include $(CONFIG_CONFIG)
include $(ARCH_CONFIG)

#############################################################################

LINUXFILE = bzImage
LINUXIMG  = $(ROOTDIR)/$(LINUXDIR)/arch/i386/boot/$(LINUXFILE)

VENDDIR    = $(ROOTDIR)/vendors/$(CONFIG_VENDOR)/$(CONFIG_PRODUCT)/.

PREFIX     = $(IMAGEDIR)/se2910-CF

# the number of 512 byte blocks in config partition (4096 = 2Mb)
CONFIG     = 4096

#############################################################################

DIRS = 

ROMFS_DIRS = \
	bin boot \
	dev dev/flash \
	etc etc/config \
	home home/httpd home/httpd/cgi-bin \
	lib lib/modules \
	mnt \
	proc \
	sbin \
	tmp \
	usr usr/bin usr/sbin \
	var var/run var/log var/tmp var/lock var/empty

DEVICES = \
	tty,c,5,0      console,c,5,1      cua0,c,5,64      cua1,c,5,65  \
	mem,c,1,1      kmem,c,1,2         null,c,1,3 \
	ram0,b,1,0     ram1,b,1,1 \
	ram2,b,1,2     ram3,b,1,3 \
	\
	ptyp0,c,2,0    ptyp1,c,2,1        ptyp2,c,2,2      ptyp3,c,2,3 \
	ptyp4,c,2,4    ptyp5,c,2,5        ptyp6,c,2,6      ptyp7,c,2,7 \
	ptyp8,c,2,8    ptyp9,c,2,9        ptypa,c,2,10     ptypb,c,2,11 \
	ptypc,c,2,12   ptypd,c,2,13       ptype,c,2,14     ptypf,c,2,15 \
	\
	tty0,c,4,0     tty1,c,4,1         tty2,c,4,2       tty3,c,4,3 \
	ttyS0,c,4,64   ttyS1,c,4,65 \
	\
	ttyp0,c,3,0    ttyp1,c,3,1        ttyp2,c,3,2      ttyp3,c,3,3 \
	ttyp4,c,3,4    ttyp5,c,3,5        ttyp6,c,3,6      ttyp7,c,3,7 \
	ttyp8,c,3,8    ttyp9,c,3,9        ttypa,c,3,10     ttypb,c,3,11 \
	ttypc,c,3,12   ttypd,c,3,13       ttype,c,3,14     ttypf,c,3,15 \
	\
	zero,c,1,5     random,c,1,8       urandom,c,1,9 \
	\
	ipsec,c,36,10 \
	ledman,c,126,0 \
	hifn0,c,28,0 \
	ppp,c,108,0 \
	nvram,c,10,144 \
	\
	hda,b,3,0 \
	hda1,b,3,1 \
	hda2,b,3,2 \
	hda3,b,3,3 \
	hda4,b,3,4 \
	hdb,b,3,64 \
	hdb1,b,3,65 \
	hdb2,b,3,66 \
	hdb3,b,3,67 \
	hdb4,b,3,68 \
	\
	mtd0,c,90,0	mtdr0,c,90,1	mtdblock0,b,31,0 \
	mtd1,c,90,2	mtdr1,c,90,3	mtdblock1,b,31,1 \
	mtd2,c,90,4	mtdr2,c,90,5	mtdblock2,b,31,2 \
	mtd3,c,90,6	mtdr3,c,90,7	mtdblock3,b,31,3 \
	mtd4,c,90,8	mtdr4,c,90,9	mtdblock4,b,31,4 \
	mtd5,c,90,10	mtdr5,c,90,11	mtdblock5,b,31,5 \
	mtd6,c,90,12	mtdr6,c,90,13	mtdblock6,b,31,6

FLASH_DEVICES = \
	ethmac,c,90,0 \
	bootarg,c,90,0 \
	boot,c,90,2 \
	image,b,3,0 \
	config,b,3,2

#############################################################################

all:
	dirs=$(DIRS) ; \
	for i in $$dirs ; do  make -C $$i || exit $? ; done

clean:
	-dirs=$(DIRS) ; \
	for i in $$dirs; do [ ! -d $$i ] || make -C $$i clean; done
	rm -f romfs.img ext3.img cramfs.img

romfs:
	[ -d $(ROMFSDIR)/$$i ] || mkdir -p $(ROMFSDIR)
	for i in $(ROMFS_DIRS); do \
		[ -d $(ROMFSDIR)/$$i ] || mkdir -p $(ROMFSDIR)/$$i; \
	done
	for i in $(DEVICES); do \
		touch $(ROMFSDIR)/dev/@$$i; \
	done
	# these permissions are needed for openpty and family to work
	# on non-ptmx ptys
	chmod 620 $(ROMFSDIR)/dev/@[pt]ty[pqrsPQRS][0-9a-f],*
	for i in $(FLASH_DEVICES); do \
		touch $(ROMFSDIR)/dev/flash/@$$i; \
	done
	$(ROMFSINST) ../../SecureEdge/romfs_x86 /
	$(ROMFSINST) /etc/lilo.conf
	$(ROMFSINST) $(LINUXIMG) /boot/vmlinuz
	$(ROMFSINST) -p 755 ../../SecureEdge/romfs_x86/etc/default/ip-up \
			/etc/default/ip-up
	$(ROMFSINST) -p 755 ../../SecureEdge/romfs_x86/etc/default/ip-down \
			/etc/default/ip-down
	$(ROMFSINST) -p 755 ../../SecureEdge/romfs_x86/etc/default/dhcpcd-eth0.exe \
			/etc/default/dhcpcd-eth0.exe
	$(ROMFSINST) -p 755 ../../SecureEdge/romfs_x86/etc/default/dhcpcd-eth1.exe \
			/etc/default/dhcpcd-eth1.exe
	$(ROMFSINST) /etc/rc
	$(ROMFSINST) /etc/fstab
	echo "cardmgr &" >> $(ROMFSDIR)/etc/default/start
	$(ROMFSINST) -s /var/tmp/log /dev/log
	echo "ttyS0::respawn:/sbin/getty 9600 -" >> $(ROMFSDIR)/etc/inittab
	echo "$(VERSIONSTR) -- " `date` > $(ROMFSDIR)/etc/version
	date '+%Y.%m.%d-%T' > $(ROMFSDIR)/etc/build-date

#############################################################################
#
# tool we need for build host based versions of the programs
#

mkcramfs: $(ROOTDIR)/user/cramfs/mkcramfs.c
	$(HOSTCC) -o $@ -I$(STAGEDIR)/include $< -lz

LILO_CFLAGS = -I$(ROOTDIR)/user/lilo -DLCF_IGNORECASE -DHAS_BOOT_H -DLCF_M386 -DLCF_READONLY -DLCF_VARSETUP -DLCF_SOLO_CHAIN -DLILO=0xbb920890

LILOSOURCE = lilo.c map.c geometry.c boot.c device.c common.c bsect.c cfg.c \
	temp.c partition.c identify.c probe.c shs2.c raid.c edit.c

.PHONY: lilo
lilo:
	gcc $(LILO_CFLAGS) -o $@ $(patsubst %.c,$(ROOTDIR)/user/lilo/%.c,$(LILOSOURCE))

#############################################################################
#
# re copy the kernel so that "make linux image" works
#

image: mkcramfs
	[ -d $(IMAGEDIR) ] || mkdir -p $(IMAGEDIR)
	$(ROMFSINST) $(LINUXIMG) /boot/vmlinuz
	rm -rf $(ROMFSDIR)/man[1-9]
	./mkcramfs -z -r $(ROMFSDIR) cramfs.img
	mknbi-linux --append=init=/bin/sh --rootmode=ro --rootdir=/dev/0100 \
			--output=$(PREFIX)-recover.bin $(LINUXIMG) cramfs.img
	genromfs -v -V "ROMdisk" -f romfs.img -d $(ROMFSDIR)
	@if [ -x /usr/local/bin/root ]; then \
		/usr/local/bin/root $(MAKE) cf-images ; \
	else \
		@echo '********************************************************' > /dev/tty ; \
		@echo '********************************************************' > /dev/tty ; \
		@echo '**                                                    **' > /dev/tty ; \
		@echo '**                                                    **' > /dev/tty ; \
		@echo '** Time to make the final compact flash image.        **' > /dev/tty ; \
		@echo '** For this you need to be "root". You will also need **' > /dev/tty ; \
		@echo '** a kernel with the loopback device enabled and ext2 **' > /dev/tty ; \
		@echo '** filesystem support.  Enter the root password below **' > /dev/tty ; \
		@echo '** if asked,  or an invalid password to abort.        **' > /dev/tty ; \
		@echo '**                                                    **' > /dev/tty ; \
		@echo '**                                                    **' > /dev/tty ; \
		@echo '********************************************************' > /dev/tty ; \
		@echo '********************************************************' > /dev/tty ; \
		@echo > /dev/tty ; \
		su root -c "$(MAKE) cf-images" < /dev/tty > /dev/tty 2>&1 ; \
	fi
	[ -n "$(NO_BUILD_INTO_TFTPBOOT)" ] || cp $(PREFIX)-recover.bin /tftpboot/.

#############################################################################
#
# make images for various sizes of compact flash,  check your geometry,
# you may need to add your own here,  remove any your don't want built from
# the cf-images target
#

.PHONY: cf-images extimage-64MB extimage-128MB

cf-images: extimage-64MB extimage-128MB

extimage-64MB:
	$(MAKE) extimage HEADS=8 SECTORS=32 CYLS=480 IMAGE=$(PREFIX)-64MB.sgu

extimage-128MB:
	$(MAKE) extimage HEADS=8 SECTORS=32 CYLS=960 IMAGE=$(PREFIX)-128MB.sgu

#############################################################################
#
# Make a lilo bootable compact flash image based on paramters passed in
#

.PHONY: extimage
extimage: lilo
	dd if=/dev/zero of=mbr.img bs=512 count=1
	printf "1,`expr $(CYLS) \\* $(HEADS) \\* $(SECTORS) - $(CONFIG) - 1`,L,*\n,,61,-\n" | sfdisk -uS -C $(CYLS) -H $(HEADS) -S $(SECTORS) mbr.img
	dd if=/dev/zero of=ext3.img bs=512 \
			count=`expr $(CYLS) \\* $(HEADS) \\* $(SECTORS) - 1 - $(CONFIG)`
	/sbin/mke2fs -F -m0 -v ext3.img
	/sbin/tune2fs -i 0 -j ext3.img
	rm -rf romfs.mnt ext3.mnt
	mkdir romfs.mnt ext3.mnt
	( \
	trap "cd $(VENDDIR); umount romfs.mnt; umount ext3.mnt; sleep 1; \
				rm -rf romfs.mnt ext3.mnt" 0; \
	mount -t romfs -o loop romfs.img romfs.mnt; \
	mount -t ext2 -o loop ext3.img ext3.mnt; \
	DEV=`mount|grep ext3.img|sed 's?.*loop=/dev/loop\([0-9]\).*$$?\1?'`;\
	echo "#Device Bios    Sectors Heads   Cyls    Start" > disktab; \
	echo "0x70$$DEV	0x80	$(SECTORS)	$(HEADS)	$(CYLS) 1" >> disktab ; \
	(cd romfs.mnt; tar cf - .) | (cd ext3.mnt; tar xf -); \
	chmod -R go-w ext3.mnt ; \
	chgrp -hR root ext3.mnt; \
	chown -hR root ext3.mnt; \
	./lilo -s /dev/null -C $(VENDDIR)/build.lilo.conf -f $(VENDDIR)/disktab; \
	rm -f $(VENDDIR)/disktab; \
	)
	cat mbr.img ext3.img | gzip -9 > $(IMAGE)
ifdef CONFIG_USER_NETFLASH_HMACMD5
	$(ROOTDIR)/user/netflash/hmacmd5 $(CONFIG_USER_NETFLASH_HMACMD5_KEY) \
				< $(IMAGE) >> $(IMAGE)
else
	$(ROOTDIR)/tools/cksum -b -o 2 $(IMAGE) >> $(IMAGE)
endif
	[ -n "$(NO_BUILD_INTO_TFTPBOOT)" ] || cp $(IMAGE) /tftpboot/.

#############################################################################
