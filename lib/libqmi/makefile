
# METHOD   = git
# VERSION  = 1.4.0
# URL      = git://anongit.freedesktop.org/libqmi                              

URL       = http://cgit.freedesktop.org/libqmi/snapshot/libqmi-1.6.0.tar.gz

CONFIGURE = autogen.sh
CONFOPTS = --without-docs --without-tests
CONFOPTS += --enable-more-warnings=no
ifdef CONFIG_LIB_GLIB_STATIC
CONFOPTS += --enable-static --disable-shared
CONFVARS = LIBS="-lgmodule-2.0 -lglib-2.0 -lffi -lz -lpthread -ldl"
endif
MAKEVARS = 
ROMFS    = libqmi_romfs

include $(ROOTDIR)/tools/automake.inc

libqmi_romfs:
ifndef CONFIG_LIB_LIBQMI_QMICLI
	$(ROMFSINST) -R /bin/qmicli
endif
ifndef CONFIG_LIB_LIBQMI_QMI_NETWORK
	$(ROMFSINST) -R /bin/qmi-network
endif
	
