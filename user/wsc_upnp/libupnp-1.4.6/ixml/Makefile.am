# $Id: Makefile.am,v 1.1.1.1 2007-07-16 03:05:29 yy Exp $
#
# "Makefile.am" for "libupnp/ixml"
#
# (C) Copyright 2005 R?mi Turboult <r3mi@users.sourceforge.net>
#

SUBDIRS 		= doc

AM_CPPFLAGS		= -I$(srcdir)/inc -I$(srcdir)/src/inc
AM_CFLAGS		= 

LDADD			= libixml.la

if ENABLE_DEBUG
    AM_CPPFLAGS		+= -DDEBUG
else
    AM_CPPFLAGS		+= -DNDEBUG
endif

lib_LTLIBRARIES		= libixml.la

libixml_la_LDFLAGS	= -version-info $(LT_VERSION_IXML) \
			  -export-symbols-regex '^ixml.*'

libixml_la_SOURCES	= \
			src/ixml.c src/node.c src/ixmlparser.c \
			src/ixmlmembuf.c src/nodeList.c \
			src/element.c src/attr.c src/document.c \
			src/namedNodeMap.c \
			src/inc/ixmlmembuf.h src/inc/ixmlparser.h

upnpincludedir		= $(includedir)/upnp
upnpinclude_HEADERS	= inc/ixml.h 


check_PROGRAMS          = test_document
TESTS                   = test/test_document.sh

test_document_SOURCES 	= test/test_document.c

EXTRA_DIST		= test/test_document.sh test/testdata

dist-hook:
	rm -rf `find $(distdir)/test/testdata/ -type f \! -name '*ml' -print`

clean-local:
	@if [ -d bin ] ; then  rm -rf bin ; fi


