
CONF_OPTS = --prefix=  --without-saslauthd --with-plugindir=/lib/sasl2
ifdef CONFIG_USER_FLATFSD_FLATFSD
CONF_OPTS += --with-configdir=/etc/config
endif

all: build
	$(MAKE) -C build

build: makefile
	rm -rf build
	mkdir build
	(cd build; ../configure $(CONFIGURE_OPTS) $(CONF_OPTS))

romfs:
	$(ROMFSINST) build/lib/.libs/libsasl2.so.2 /lib/
	$(ROMFSINST) -d build/plugins/.libs/libgssapiv2.so /lib/sasl2/libgssapiv2.so
	
clean:
	rm -rf build

