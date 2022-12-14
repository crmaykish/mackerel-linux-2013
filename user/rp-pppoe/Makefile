# Generated automatically from Makefile.in by configure.
# Generated automatically from Makefile.in by configure.
#***********************************************************************
#
# Makefile
#
# Makefile for Roaring Penguin's Linux user-space PPPoE client.
#
# Copyright (C) 2000 Roaring Penguin Software Inc.
#
# This program may be distributed according to the terms of the GNU
# General Public License, version 2 or (at your option) any later version.
#
# $Id: Makefile,v 1.18 2003-09-30 00:24:05 steveb Exp $
#***********************************************************************

# Version is set ONLY IN THE MAKEFILE!  Don't delete this!
VERSION=2.3

prefix=
exec_prefix=${prefix}
mandir=${prefix}/man
docdir=/usr/doc/rp-pppoe-$(VERSION)
install=/usr/bin/install -c
install_dir=/usr/bin/install -c -d
sbindir=${exec_prefix}/bin

# Program paths
PPPOE_PATH=$(sbindir)/pppoe
PPPD_PATH=/bin/pppd

# Configuration file paths
ifdef CONFIG_USER_FLATFSD_FLATFSD
PPPOESERVER_PPPD_OPTIONS=/etc/config/pppoe-server-options
else
PPPOESERVER_PPPD_OPTIONS=/etc/pppoe-server-options
endif

PATHS='-DPPPOE_PATH="$(PPPOE_PATH)"' '-DPPPD_PATH="$(PPPD_PATH)"' \
	'-DPPPOE_SERVER_OPTIONS="$(PPPOESERVER_PPPD_OPTIONS)"'

#CFLAGS= -g -O2 -Wall -Wstrict-prototypes -ansi -pedantic $(DEFINES) $(PATHS)
CFLAGS+=$(DEFINES) $(PATHS)
LIBS=$(LDLIBS)

PROGS=pppoe pppoe-sniff 
ifdef CONFIG_USER_RP_PPPOE_PPPOE_SERVER
PROGS += pppoe-server
endif

FLTFLAGS += -s 10000
all: $(PROGS)
	@echo ""
	@echo "Type 'make install' as root to install the software."

pppoe-sniff: pppoe-sniff.o if.o common.o
	$(CC) $(LDFLAGS) -o $@ pppoe-sniff.o if.o common.o $(LIBS)

pppoe-server: pppoe-server.o if.o debug.o common.o md5.o
	$(CC) $(LDFLAGS) -o $@ pppoe-server.o if.o debug.o common.o md5.o $(LIBS)

pppoe: pppoe.o if.o debug.o common.o ppp.o
	$(CC) $(LDFLAGS) -o $@ pppoe.o if.o debug.o common.o ppp.o $(LIBS)

pppoe.o: pppoe.c pppoe.h
	$(CC) $(CFLAGS) '-DVERSION="$(VERSION)"' -c -o pppoe.o pppoe.c

ppp.o: ppp.c pppoe.h
	$(CC) $(CFLAGS) '-DVERSION="$(VERSION)"' -c -o ppp.o ppp.c

md5.o: md5.c md5.h
	$(CC) $(CFLAGS) '-DVERSION="$(VERSION)"' -c -o md5.o md5.c

pppoe-server.o: pppoe-server.c pppoe.h
	$(CC) $(CFLAGS) '-DVERSION="$(VERSION)"' -c -o pppoe-server.o pppoe-server.c

pppoe-sniff.o: pppoe-sniff.c pppoe.h
	$(CC) $(CFLAGS) '-DVERSION="$(VERSION)"' -c -o pppoe-sniff.o pppoe-sniff.c

if.o: if.c pppoe.h
	$(CC) $(CFLAGS) '-DVERSION="$(VERSION)"' -c -o if.o if.c

common.o: common.c pppoe.h
	$(CC) $(CFLAGS) '-DVERSION="$(VERSION)"' -c -o common.o common.c

debug.o: debug.c pppoe.h
	$(CC) $(CFLAGS) '-DVERSION="$(VERSION)"' -c -o debug.o debug.c

install: pppoe pppoe-server pppoe-sniff
	-mkdir -p $(RPM_INSTALL_ROOT)$(sbindir)
	$(install) -m 755 -s pppoe $(RPM_INSTALL_ROOT)$(sbindir)
	$(install) -m 755 -s pppoe-server $(RPM_INSTALL_ROOT)$(sbindir)
	$(install) -m 755 -s pppoe-sniff $(RPM_INSTALL_ROOT)$(sbindir)
	$(install) -m 755 adsl-connect adsl-start adsl-status adsl-stop adsl-setup $(RPM_INSTALL_ROOT)$(sbindir)
	-mkdir -p $(RPM_INSTALL_ROOT)$(docdir)
	$(install) -m 644 CHANGES HOW-TO-CONNECT LICENSE README firewall pap-secrets adsl-state.pl $(RPM_INSTALL_ROOT)$(docdir)
	-mkdir -p $(RPM_INSTALL_ROOT)$(mandir)/man8
	$(install) -m 644 pppoe.8 adsl-start.8 adsl-stop.8 adsl-status.8 adsl-connect.8 adsl-setup.8 pppoe-server.8 pppoe-sniff.8 $(RPM_INSTALL_ROOT)$(mandir)/man8
	-mkdir -p $(RPM_INSTALL_ROOT)$(mandir)/man5
	$(install) -m 644 pppoe.conf.5 $(RPM_INSTALL_ROOT)$(mandir)/man5
	-mkdir -p $(RPM_INSTALL_ROOT)/etc/ppp
	@for i in pppoe.conf firewall-standalone firewall-masq ; do \
		if [ ! -f $(RPM_INSTALL_ROOT)/etc/ppp/$$i ] ; then \
			$(install) -m 644 $$i $(RPM_INSTALL_ROOT)/etc/ppp ; \
		else \
			echo "NOT overwriting existing $(RPM_INSTALL_ROOT)/etc/ppp/$$i" ;\
			$(install) -m 644 $$i $(RPM_INSTALL_ROOT)/etc/ppp/$$i-$(VERSION) ;\
		fi ;\
	done
	$(install) -m 644 pppoe-server-options $(RPM_INSTALL_ROOT)$(PPPOESERVER_PPPD_OPTIONS)
	@if [ -f /etc/redhat-release ] ; then \
		echo "Looks like a Red Hat system; installing $(RPM_INSTALL_ROOT)/etc/rc.d/init.d/adsl" ; \
		mkdir -p $(RPM_INSTALL_ROOT)/etc/rc.d/init.d ;\
		$(install) -m 755 adsl-init $(RPM_INSTALL_ROOT)/etc/rc.d/init.d/adsl ; \
	fi
	@if [ -f /etc/SuSE-release ] ; then \
		echo "Looks like a SuSE Linux system; installing $(RPM_INSTALL_ROOT)/etc/rc.d/init.d/adsl" ; \
		mkdir -p $(RPM_INSTALL_ROOT)/etc/rc.d/init.d ;\
		$(install) -m 755 adsl-init-suse $(RPM_INSTALL_ROOT)/etc/rc.d/init.d/adsl ; \
	fi
	@echo ""
	@echo "Type 'adsl-setup' to configure the software."

distro:
	rm -rf rp-pppoe-$(VERSION)
	mkdir rp-pppoe-$(VERSION)

	cp CHANGES HOW-TO-CONNECT LICENSE Makefile.in PROBLEMS README \
	adsl-connect.8 adsl-connect.in adsl-init.in adsl-init-suse.in adsl-setup.8 \
	adsl-setup.in adsl-start.8 adsl-start.in adsl-state.pl adsl-status \
	adsl-stop.8 adsl-status.8 adsl-stop.in config.h.in configure configure.in \
	firewall firewall-masq firewall-standalone go install-sh \
	pap-secrets pppoe.8 pppoe.conf pppoe.conf.5 pppoe-server-options \
	acconfig.h common.c config.h debug.c if.c md5.c md5.h \
	ppp.c pppoe-server.c pppoe-server.8 pppoe.c pppoe.h \
	pppoe-sniff.c pppoe-sniff.8 \
	rp-pppoe.spec rp-pppoe-$(VERSION)

	tar cvf rp-pppoe-$(VERSION).tar rp-pppoe-$(VERSION)/*
	gzip -f -v -9 rp-pppoe-$(VERSION).tar

romfs:
	$(ROMFSINST) /bin/pppoe
	$(ROMFSINST) -e CONFIG_USER_RP_PPPOE_PPPOE_SERVER /bin/pppoe-server

clean:
	rm -f *.gdb *.elf *.o pppoe pppoe-sniff core *~

distclean: clean
	rm -f Makefile config.h config.cache config.log config.status
	rm -f adsl-connect adsl-start adsl-stop adsl-init adsl-setup

.PHONY: clean

.PHONY: distclean
