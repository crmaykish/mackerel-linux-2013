# $Id: Makefile.am,v 1.1.1.1 2007-07-16 03:05:29 yy Exp $
#
# Top-level "Makefile.am" for libupnp
#
# Copyright (C) 2005 R?mi Turboult <r3mi@users.sourceforge.net>
#

ACLOCAL_AMFLAGS = -I m4

DISTCHECK_CONFIGURE_FLAGS = --enable-debug --enable-samples

SUBDIRS = ixml threadutil upnp docs/dist


EXTRA_DIST = \
	libupnp.pc.in \
	LICENSE \
	THANKS \
	libupnp.spec \
	build/libupnp.bpf \
	build/libupnp.bpr \
	build/libupnp.dsp \
	build/libupnp.dsw \
	build/inc/autoconfig.h \
	build/inc/config.h \
	build/inc/upnpconfig.h


# This variable must have 'exec' in its name, in order to be installed 
# by 'install-exec' target (instead of default 'install-data')
pkgconfigexecdir = $(libdir)/pkgconfig
pkgconfigexec_DATA = libupnp.pc

$(pkgconfigexec_DATA): config.status


if WITH_DOCUMENTATION
    doc_DATA = LICENSE README NEWS TODO THANKS
endif


CLEANFILES = IUpnpErrFile.txt IUpnpInfoFile.txt

