
URL = http://www.keepalived.org/software/keepalived-1.2.7.tar.gz

ifdef CONFIG_USER_KEEPALIVED_MINIMAL
AUTOMAKE_ROMFS = my_romfs
endif

include $(ROOTDIR)/tools/automake.inc

ifdef CONFIG_USER_KEEPALIVED_MINIMAL
my_romfs:
	$(ROMFSINST) build/keepalived-*-install/sbin/keepalived /sbin/keepalived
endif
