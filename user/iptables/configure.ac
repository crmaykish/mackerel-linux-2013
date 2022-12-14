
AC_INIT([iptables], [1.4.3.2])

# See libtool.info "Libtool's versioning system"
libxtables_vcurrent=2
libxtables_vage=0

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_INSTALL
AM_INIT_AUTOMAKE([-Wall])
AC_PROG_CC
AM_PROG_CC_C_O
AC_DISABLE_STATIC
AM_PROG_LIBTOOL

AC_ARG_WITH([kernel],
	AS_HELP_STRING([--with-kernel=PATH],
	[Path to kernel source/build directory]),
	[kbuilddir="$withval"; ksourcedir="$withval";])
AC_ARG_WITH([kbuild],
	AS_HELP_STRING([--with-kbuild=PATH],
	[Path to kernel build directory [[/lib/modules/CURRENT/build]]]),
	[kbuilddir="$withval"])
AC_ARG_WITH([ksource],
	AS_HELP_STRING([--with-ksource=PATH],
	[Path to kernel source directory [[/lib/modules/CURRENT/source]]]),
	[ksourcedir="$withval"])
AC_ARG_WITH([xtlibdir],
	AS_HELP_STRING([--with-xtlibdir=PATH],
	[Path where to install Xtables extensions [[LIBEXECDIR/xtables]]]),
	[xtlibdir="$withval"],
	[xtlibdir="${libexecdir}/xtables"])
AC_ARG_ENABLE([ipv4],
	AS_HELP_STRING([--disable-ipv4], [Do not build iptables]),
	[enable_ipv4="$enableval"], [enable_ipv4="yes"])
AC_ARG_ENABLE([ipv6],
	AS_HELP_STRING([--disable-ipv6], [Do not build ip6tables]),
	[enable_ipv6="$enableval"], [enable_ipv6="yes"])
AC_ARG_ENABLE([devel],
	AS_HELP_STRING([--enable-devel],
	[Install Xtables development headers]),
	[enable_devel="$enableval"], [enable_devel="yes"])
AC_ARG_ENABLE([libipq],
	AS_HELP_STRING([--enable-libipq], [Build and install libipq]))
AC_ARG_WITH([pkgconfigdir], AS_HELP_STRING([--with-pkgconfigdir=PATH],
	[Path to the pkgconfig directory [[LIBDIR/pkgconfig]]]),
	[pkgconfigdir="$withval"], [pkgconfigdir='${libdir}/pkgconfig'])

AC_CHECK_HEADER([linux/dccp.h])

blacklist_modules="";
if test "$ac_cv_header_linux_dccp_h" != "yes"; then
	blacklist_modules="$blacklist_modules dccp";
fi;
AC_SUBST([blacklist_modules])

AM_CONDITIONAL([ENABLE_STATIC], [test "$enable_static" = "yes"])
AM_CONDITIONAL([ENABLE_SHARED], [test "$enable_shared" = "yes"])
AM_CONDITIONAL([ENABLE_IPV4], [test "$enable_ipv4" = "yes"])
AM_CONDITIONAL([ENABLE_IPV6], [test "$enable_ipv6" = "yes"])
AM_CONDITIONAL([ENABLE_DEVEL], [test "$enable_devel" = "yes"])
AM_CONDITIONAL([ENABLE_LIBIPQ], [test "$enable_libipq" = "yes"])

regular_CFLAGS="-D_LARGEFILE_SOURCE=1 -D_LARGE_FILES -D_FILE_OFFSET_BITS=64 \
	-D_REENTRANT -Wall -Waggregate-return -Wmissing-declarations \
	-Wmissing-prototypes -Wredundant-decls -Wshadow -Wstrict-prototypes \
	-Winline -pipe \
	-DXTABLES_LIBDIR=\\\"\${xtlibdir}\\\" -DXTABLES_INTERNAL";
kinclude_CFLAGS="";
if [[ "$enable_static" = yes ]]; then
	regular_CFLAGS="$regular_CFLAGS -DNO_SHARED_LIBS=1";
fi
if [[ -n "$kbuilddir" ]]; then
	kinclude_CFLAGS="$kinclude_CFLAGS -I $kbuilddir/include";
fi;
if [[ -n "$ksourcedir" ]]; then
	kinclude_CFLAGS="$kinclude_CFLAGS -I $ksourcedir/include";
fi;

AC_SUBST([regular_CFLAGS])
AC_SUBST([kinclude_CFLAGS])
AC_SUBST([kbuilddir])
AC_SUBST([ksourcedir])
AC_SUBST([xtlibdir])
AC_SUBST([pkgconfigdir])
AC_SUBST([libxtables_vcurrent])
AC_SUBST([libxtables_vage])
libxtables_vmajor=$(($libxtables_vcurrent - $libxtables_vage));
AC_SUBST([libxtables_vmajor])

AC_CONFIG_FILES([Makefile extensions/GNUmakefile include/Makefile
	libipq/Makefile
	include/xtables.h include/iptables/internal.h libiptc.pc xtables.pc])
AC_OUTPUT
