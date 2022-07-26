# Makefile to auto build tarball's with patches etc
# see Documentation/automake.txt for more info on using this file
# Copyright (C) 2009 David McCullough <davidm@uclinux.org>

.EXPORT_ALL_VARIABLES:

AT := $(if $(V:0=),,@)
ifndef STAGEDIR
STAGEDIR = $(ROOTDIR)
endif

#
# You can override the downloads cache dir in case you would prefer
# have all the files in tree or elsewhere.  The default is ~/.downloads
#
ifndef UC_DOWNLOADS
UC_DOWNLOADS = $(HOME)/.downloads
endif

#
# these config options apply to all things we build
#
CONFOPTS += --prefix=
ifdef CONFIG_USER_FLATFSD_FLATFSD
ifeq ($(findstring --sysconfdir,$(CONFOPTS)),)
  CONFOPTS += --sysconfdir=/etc/config
endif
endif

CONFOPTS += --with-vendor-name=$(CONFIG_VENDOR)
CONFOPTS += --with-vendor-name-short=$(CONFIG_VENDOR)
CONFOPTS += --with-os-name=uClinux-dist
CONFOPTS += --with-os-vendor=uClinux
CONFOPTS += --disable-static

#
# for uClibc without glibc compat
#
CONFOPTS += --enable-malloc0returnsnull

#
# make sure LD does the right thing during configure if invoked directly
#
LDFLAGS  += -Wl,-L$(STAGEDIR)/lib -Wl,-rpath-link -Wl,$(STAGEDIR)/lib
LDFLAGS  += -Wl,-L$(STAGEDIR)/lib64 -Wl,-rpath-link -Wl,$(STAGEDIR)/lib64

#
# where our pkgconfig stuff will go
#
PKG_CONFIG_PATH   = $(STAGEDIR)/lib/pkgconfig:$(STAGEDIR)/lib:$(STAGEDIR)/share/pkgconfig
PKG_CONFIG_LIBDIR = $(STAGEDIR)/lib/pkgconfig

#
# its generally a bad idea to fail on linker warnings for packages we
# aren't fully controlling,  though we could put this back conditionally
#
LDFLAGS := $(LDFLAGS:-Wl,--fatal-warnings=)

BUILD_CC = $(HOSTCC)
HOST_CC      = gcc
HOST_CXX     = g++
HOST_RANLIB  = $(shell which ranlib || echo :)

#
# we use wget to download
#
WGET ?= wget
GIT ?= git
METHOD ?= default

#
# force using our special libtool
#
MAKEVARS     ?= LIBTOOL="$(LIBTOOL)"

#
# remap directories in a build to ensure that only uClinux-dist files
# are used
#
UCFRONT_MAPDIRS  = /lib/=$(STAGEDIR)/lib/
UCFRONT_MAPDIRS += /include/=$(STAGEDIR)/include/
ifneq ($(STAGEDIR),$(ROOTDIR))
UCFRONT_MAPDIRS += /usr/include=$(STAGEDIR)/usr/include
UCFRONT_MAPDIRS += /usr/lib=$(STAGEDIR)/usr/lib
else
UCFRONT_MAPDIRS += /usr/include=$(STAGEDIR)/include
UCFRONT_MAPDIRS += /usr/lib=$(STAGEDIR)/lib
endif

#
# some short cut config,  just set URL = blah
# and we work the rest out
#
ifeq ($(AUTOMAKE_y)$(AUTOMAKE_n)$(AUTOMAKE_),)
  ifdef URL
    ifndef PKG
      PKG := $(shell basename $(URL))
      PKG := $(PKG:.tar.gz=)
      PKG := $(PKG:.tar.bz2=)
      PKG := $(PKG:.tar.xz=)
      PKG := $(PKG:.tgz=)
      PKG := $(PKG:.tbz2=)
      PKG := $(PKG:.txz=)
      PKG := $(PKG:.zip=)
      PKG := $(PKG:.git=)
    endif
    AUTOMAKE_y += $(PKG)
  endif
endif

define pkg_def
  $(1)_URL ?= $(URL)
  $(1)_METHOD ?= $(METHOD)
  $(1)_INSTALL ?= $(if $(INSTALL),$(INSTALL),install)
  $(1)_NODEFCONF ?= $(NODEFCONF)
  $(1)_AUTOCONF ?= $(AUTOCONF)
  $(1)_VERSION ?= $(VERSION)
  $(1)_SUBDIR ?= $(SUBDIR)
  $(1)_ROMFS ?= $(ROMFS)
  $(1)_CONFIGURE ?= $(if $(CONFIGURE),$(CONFIGURE),configure)
  $(1)_BUILDDIR ?= $(if $(BUILDDIR),$(BUILDDIR),$(1))
  $(1)_SRCDIR ?= $(if $(SRCDIR),$(SRCDIR),$$($(1)_BUILDDIR))
  $(1)_DOWNLOADNAME ?= $(if $(DOWNLOADNAME),$(DOWNLOADNAME),$$(shell basename "$$($(1)_URL)"))
endef
$(foreach pkg,$(AUTOMAKE_y),$(eval $(call pkg_def,$(pkg))))

# put arguments on their own line
define oneline
$(1)
endef

#
# allow some targets to be overridden
#
BUILDTARGET ?= built
INSTALLTARGET ?= installed
FINALTARGET ?= lndir
COMPLETETARGET ?= complete_target

#
# the main target
#
all: $(foreach pkg,$(AUTOMAKE_y),build/$(pkg)-$(FINALTARGET)) $(COMPLETETARGET)

.PHONY: $(COMPLETETARGET)
$(COMPLETETARGET): $(foreach pkg,$(AUTOMAKE_y),build/$(pkg)-$(FINALTARGET))

.PHONY: force_target
force_target:

.PRECIOUS: $(addprefix build/,$(addsuffix -extracted,$(AUTOMAKE_y)))
.PRECIOUS: $(addprefix build/,$(addsuffix -autoconf,$(AUTOMAKE_y)))
.PRECIOUS: $(addprefix build/,$(addsuffix -configured,$(AUTOMAKE_y)))
.PRECIOUS: $(addprefix build/,$(addsuffix -built,$(AUTOMAKE_y)))
.PRECIOUS: $(addprefix build/,$(addsuffix -installed,$(AUTOMAKE_y)))
.PRECIOUS: $(addprefix build/,$(addsuffix -patched,$(AUTOMAKE_y)))
.PRECIOUS: $(addprefix build/,$(addsuffix -$(BUILDTARGET),$(AUTOMAKE_y)))
.PRECIOUS: $(addprefix build/,$(addsuffix -$(INSTALLTARGET),$(AUTOMAKE_y)))

#
# download support for tarballs and the like from a URL
#
define download_dep_default

.PRECIOUS: downloads/$$($(1)_DOWNLOADNAME)
downloads/$$($(1)_DOWNLOADNAME):
	@echo "Downloading $$($(1)_DOWNLOADNAME) from $($(1)_URL) ..."
	$(AT)[ -d "$(UC_DOWNLOADS)" ] || mkdir -p "$(UC_DOWNLOADS)"
	$(AT)if [ ! -f "$(UC_DOWNLOADS)/`basename $$@`" ]; then \
		cd "$(UC_DOWNLOADS)"; $(WGET) -O `basename $$@` "$($(1)_URL)"; \
	fi
	$(AT)mkdir -p `dirname $$@`
	$(AT)if [ -f "$(UC_DOWNLOADS)/`basename $$@`" ]; then \
			ln -fs "$(UC_DOWNLOADS)/`basename $$@`" $$@; \
	fi
	$(AT)if [ ! -f "$$@" ]; then \
		echo "Cannot find download for $$@" >&2 ; \
		exit 1; \
	fi

build/$(1)-extracted: downloads/$($(1)_DOWNLOADNAME)
	@echo "Extracting $$< ..."
	$(AT)rm -rf build/$($(1)_SRCDIR)
	$(AT)mkdir -p build
	$(AT)case "$$<" in \
	*zip) rm -rf build/$(1); mkdir -p build/$(1); pkg=`pwd`/$$<; (cd build/$(1); unzip $$$$pkg);; \
	*bz2) bunzip2 < $$< | (cd build; tar xf -);; \
	*xz) unxz < $$< | (cd build; tar xf -);; \
	*) gunzip < $$< | (cd build; tar xf -);; \
	esac || exit 1
	$(AT)touch $$@

endef

#
# Support for remote GIT subdirectories of sorts, we do not remember master
# downloads as they change from load to load so just save it until the next
# clean.  The clone will save us downloading it all again.
#
define download_dep_gitarchive

.PRECIOUS: downloads/$(1)$(if $($(1)_VERSION),-$($(1)_VERSION)).tar
downloads/$(1)$(if $($(1)_VERSION),-$($(1)_VERSION)).tar:
	@echo "Downloading `basename $$@` ..."
	$(AT)[ -d "$(UC_DOWNLOADS)" ] || mkdir -p "$(UC_DOWNLOADS)"
	$(AT)if [ ! -f "$(UC_DOWNLOADS)/`basename $$@`" ]; then \
		echo "Updating/Cloning `basename $$($(1)_URL)` for archive ..."; \
		if [ -d "archive/`basename $$($(1)_URL)`" ]; then \
			(\
			cd "archive/`basename $$($(1)_URL)`"; \
			$(GIT) checkout -q master || exit 1; \
			$(GIT) pull || exit 1; \
			$(GIT) checkout -q $(if $($(1)_VERSION),$($(1)_VERSION),master) || exit 1; \
			) || exit 1; \
		else \
			$(GIT) clone $$($(1)_URL) archive/`basename $$($(1)_URL)` || exit 1; \
			(\
			cd "archive/`basename $$($(1)_URL)`"; \
			$(GIT) checkout -q $(if $($(1)_VERSION),$($(1)_VERSION),master) || exit 1; \
			) || exit 1; \
		fi ; \
		cd archive/`basename $$($(1)_URL)`; \
			$(GIT) archive -o $(UC_DOWNLOADS)/`basename $$@` HEAD:$(if $($(1)_SUBDIR),$($(1)_SUBDIR)/,) || exit 1; \
	fi
	$(AT)mkdir -p `dirname $$@`
	$(AT)if [ "$(if $($(1)_VERSION),$($(1)_VERSION),master)" = "master" ]; then \
			mv -f "$(UC_DOWNLOADS)/`basename $$@`" $$@ || exit 1; \
	elif [ -f "$(UC_DOWNLOADS)/`basename $$@`" ]; then \
			ln -fs "$(UC_DOWNLOADS)/`basename $$@`" $$@; \
	fi
	$(AT)if [ ! -f "$$@" ]; then \
		echo "Cannot find download for $$@" >&2 ; \
		exit 1; \
	fi

build/$(1)-extracted: downloads/$(1)$(if $($(1)_VERSION),-$($(1)_VERSION)).tar
	@echo "Extracting $$< ..."
	$(AT)rm -rf build/$(1)
	$(AT)mkdir -p build/$(1)
	$(AT)cat < $$< | (cd build/$(1); tar xf -); exit $?
	$(AT)touch $$@

endef

#
# Support for remote GIT repos
#
define download_dep_git

build/$(1)-extracted:
	@echo "Cloning $(1) ..."
	$(AT)mkdir -p clone
	$(AT)if [ -d "clone/$(1)" ]; then \
		cd "clone/$(1)"; \
		$(GIT) checkout -q master; \
		$(GIT) pull || exit 1; \
	else \
		$(GIT) clone $$($(1)_URL) clone/$(1) || exit 1; \
	fi
	$(AT)cd "clone/$(1)" &&	$(GIT) checkout -q $(if $($(1)_VERSION),$($(1)_VERSION),master)
	$(AT)rm -rf build/$(1)
	$(AT)mkdir -p build/$(1)
	$(AT)lndir -silent `pwd`/clone/$(1) build/$(1)
	$(AT)touch $$@

endef

$(foreach pkg,$(AUTOMAKE_y),$(eval $(call download_dep_$($(pkg)_METHOD),$(pkg))))

$(foreach pkg,$(AUTOMAKE_y), $(eval $(call oneline,build/$(pkg)-extracted: $(wildcard patches/series patches/$(pkg)*.patch) $(MAKEFILE_LIST))))

build/%-patched: build/%-extracted
	$(AT)if [ -f patches/series ]; then \
		while read t; do \
			[ -f patches/$$t ] || continue; \
			echo "Patching $* with patches/$$t from series"; \
			case "$$t" in \
			*.gz) (cd build/$($*_SRCDIR); gunzip | patch -E -p1) < patches/$$t || exit 1;; \
			*)    (cd build/$($*_SRCDIR); patch -E -p1) < patches/$$t || exit 1;; \
			esac || exit 1; \
		done < patches/series || exit 1; \
	else \
		for t in patches/$**.patch*; do \
			[ -f "$$t" ] || continue; \
			echo "Patching $* with $$t"; \
			case "$$t" in \
			*.gz) (cd build/$($*_SRCDIR); gunzip | patch -E -p1) < $$t || exit 1 ;; \
			*)    (cd build/$($*_SRCDIR); patch -E -p1) < $$t || exit 1;; \
			esac || exit 1; \
		done || exit 1; \
	fi
	$(AT)touch $@

build/%-autoconf: build/%-patched
	@echo "Autoconf $* ..."
	$(if $($*_AUTOCONF),cd build/$($*_SRCDIR); $($*_AUTOCONF))
	$(AT)touch $@

build/%-configured: build/%-autoconf
	@echo "Configure $* ..."
	$(AT)mkdir -p build/$($*_BUILDDIR)
	$(AT)cd build/$($*_BUILDDIR); \
		if [ -f $($*_CONFIGURE) ]; then \
			$(CONFVARS) \
			$($*_CONFVARS) \
			CFLAGS="$(CFLAGS) $($*_CFLAGS)" \
			bash $($*_CONFIGURE) $(if $($*_NODEFCONF),,$(CONFIGURE_OPTS) $(CONFOPTS)) $($*_CONFOPTS); \
		fi
	$(AT)touch $@

build/%-built: build/%-configured
	@echo "Building $* ..."
	$(AT)$(BUILDVARS) \
	$($*_BUILDVARS) \
		$(MAKE) -C build/$($*_BUILDDIR) $(MAKEVARS) $($*_MAKEVARS)
	$(AT)touch $@

build/%-installed: build/%-$(BUILDTARGET)
	@echo "Installing $* ..."
	$(AT)mkdir -p build/$*-install
	$(AT)$(BUILDVARS) \
	$($*_BUILDVARS) \
	$($*_INSTALLVARS) \
		$(MAKE) -C build/$($*_BUILDDIR) $($*_INSTALL) DESTDIR=`pwd`/build/$*-install $(MAKEVARS) $($*_MAKEVARS)
	$(AT)touch $@

build/%-lndir: build/%-$(INSTALLTARGET) force_target
	@echo "Staging $* ..."
	$(AT)mkdir -p build/$*-install
	$(AT)cd build/$*-install; find . \( -type f -o -type l \) | egrep -v '.la$$' | \
	while read t; do \
		case "$$t" in \
		./bin/*) continue;; \
		./sbin/*) continue;; \
		esac; \
		rm -f $(STAGEDIR)/$$t; \
		mkdir -p "`dirname \"$(STAGEDIR)/$$t\"`"; \
		ln -s "`pwd`/$$t" "$(STAGEDIR)/$$t"; \
		echo "ln -s \"`pwd`/$$t\" \"$(STAGEDIR)/$$t\""; \
	done
	$(AT)touch $@

clean: $(foreach pkg,$(AUTOMAKE_y),$($(pkg)_CLEAN))
	rm -rf build downloads

romfs_pre:
	@echo "Romfs pre-install ..."
	$(AT)for i in build/*-install; do \
		echo "Installing `basename $$i -install` ..."; \
		[ ! -d $$i/usr ] || $(ROMFSINST) -v -f $$i/usr /usr; \
		[ ! -d $$i/bin ] || $(ROMFSINST) -v -f $$i/bin /bin; \
		[ ! -d $$i/sbin ] || $(ROMFSINST) -v -f $$i/sbin /sbin; \
		[ ! -d $$i/lib ] || $(ROMFSINST) -v -f $$i/lib /lib; \
		[ ! -d $$i/lib64 ] || $(ROMFSINST) -v -f $$i/lib64 /lib; \
		[ ! -d $$i/libexec ] || $(ROMFSINST) -v -f $$i/libexec /libexec; \
		rm -rf $$i/etc/default; \
		[ ! -d $$i/etc/config ] || mv $$i/etc/config $$i/etc/default; \
		[ ! -d $$i/etc ] || $(ROMFSINST) -v -f $$i/etc /etc; \
		[ ! -d $$i/share ] || $(ROMFSINST) -v -f $$i/share /share; \
	done
	rm -f $(ROMFSDIR)/lib/*.a
	rm -f $(ROMFSDIR)/lib/*.la

romfs_post: $(foreach pkg,$(AUTOMAKE_y),$($(pkg)_ROMFS))

romfs: $(if $(AUTOMAKE_ROMFS),$(AUTOMAKE_ROMFS),romfs_pre) romfs_post

