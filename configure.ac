AC_INIT(libvirt-sandbox, 0.6.1)
AC_CONFIG_SRCDIR(libvirt-sandbox/libvirt-sandbox.h)
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
dnl Make automake keep quiet about wildcards & other GNUmake-isms
AM_INIT_AUTOMAKE([-Wno-portability])
AM_MAINTAINER_MODE([enable])
AC_CANONICAL_HOST

AM_SILENT_RULES([yes])

GIO_UNIX_REQUIRED=2.28.0
GOBJECT_REQUIRED=2.32.0
LIBVIRT_REQUIRED=1.0.2
LIBVIRT_GCONFIG_REQUIRED=0.2.1
LIBVIRT_GLIB_REQUIRED=0.2.2
LIBVIRT_GOBJECT_REQUIRED=0.1.7
GOBJECT_INTROSPECTION_REQUIRED=0.10.8
LZMA_REQUIRED=5.0.0
ZLIB_REQUIRED=1.2.0

LIBVIRT_SANDBOX_MAJOR_VERSION=`echo $VERSION | awk -F. '{print $1}'`
LIBVIRT_SANDBOX_MINOR_VERSION=`echo $VERSION | awk -F. '{print $2}'`
LIBVIRT_SANDBOX_MICRO_VERSION=`echo $VERSION | awk -F. '{print $3}'`
LIBVIRT_SANDBOX_VERSION=$LIBVIRT_SANDBOX_MAJOR_VERSION.$LIBVIRT_SANDBOX_MINOR_VERSION.$LIBVIRT_SANDBOX_MICRO_VERSION

# In libtool terminology LIBVIRT_VERSION_INFO corresponds
# to  CURRENT:REVISION:AGE
#
# CURRENT
#     The most recent interface number that this library implements.
#
# REVISION
#     The implementation number of the CURRENT interface.
#
# AGE
#     The difference between the newest and oldest interfaces that this
#     library implements.
#
# In other words, the library implements all the interface numbers
# in the range from number `CURRENT - AGE' to `CURRENT'.
#
# Libtool assigns the soname version from `CURRENT - AGE'. We started
# out at zero, but for the time being, we're not ABI stable, so we can
# let SONUM change.
#
# We would, like the full libvirt-sandbox version number reflected
# in the so version'd symlinks, and this is based on AGE.REVISION
#
# Until we declare ABI stability, this can be toggled upon release
LIBVIRT_SANDBOX_SONUM=5

AGE=`expr $LIBVIRT_SANDBOX_MAJOR_VERSION '*' 1000 + $LIBVIRT_SANDBOX_MINOR_VERSION`
REVISION=$LIBVIRT_SANDBOX_MICRO_VERSION
CURRENT=`expr $LIBVIRT_SANDBOX_SONUM + $AGE`
LIBVIRT_SANDBOX_VERSION_INFO=$CURRENT:$REVISION:$AGE

AC_SUBST([LIBVIRT_SANDBOX_MAJOR_VERSION])
AC_SUBST([LIBVIRT_SANDBOX_MINOR_VERSION])
AC_SUBST([LIBVIRT_SANDBOX_MICRO_VERSION])
AC_SUBST([LIBVIRT_SANDBOX_SONUM])
AC_SUBST([LIBVIRT_SANDBOX_VERSION])
AC_SUBST([LIBVIRT_SANDBOX_VERSION_INFO])
AC_SUBST([LIBVIRT_SANDBOX_VERSION_NUMBER])

AC_PROG_CC
AM_PROG_CC_C_O

AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

AC_CONFIG_LIBOBJ_DIR([libvirt-sandbox])

LIBVIRT_SANDBOX_COMPILE_WARNINGS

PKG_CHECK_MODULES(GIO_UNIX, gio-unix-2.0 >= $GIO_UNIX_REQUIRED)
PKG_CHECK_MODULES(GOBJECT, gobject-2.0 >= $GOBJECT_REQUIRED)
PKG_CHECK_MODULES(LIBVIRT, libvirt >= $LIBVIRT_REQUIRED)
PKG_CHECK_MODULES(LIBVIRT_GLIB, libvirt-glib-1.0 >= $LIBVIRT_GOBJECT_REQUIRED)
PKG_CHECK_MODULES(LIBVIRT_GOBJECT, libvirt-gobject-1.0 >= $LIBVIRT_GOBJECT_REQUIRED)
PKG_CHECK_MODULES(LIBVIRT_GCONFIG, libvirt-gconfig-1.0 >= $LIBVIRT_GCONFIG_REQUIRED)

AC_ARG_WITH([zlib],
  [AS_HELP_STRING([--with-zlib],
    [add ZLIB support @<:@default=yes@:>@])])
m4_divert_text([DEFAULTS], [with_zlib=yes])

orig_pkg_config="$PKG_CONFIG"
PKG_CONFIG="$PKG_CONFIG --static"
if test "$with_zlib" = "yes" ; then
    PKG_CHECK_MODULES(ZLIB, zlib >= $ZLIB_REQUIRED)
    AC_DEFINE([WITH_ZLIB], [1], [Whether ZLIB support was enabled])
fi

AC_ARG_WITH([lzma],
  [AS_HELP_STRING([--with-lzma],
    [add LZMA support @<:@default=yes@:>@])])
m4_divert_text([DEFAULTS], [with_lzma=yes])

if test "$with_lzma" = "yes" ; then
    PKG_CHECK_MODULES(LZMA, liblzma >= $LZMA_REQUIRED)
    AC_DEFINE([WITH_LZMA], [1], [Whether LZMA support was enabled])
fi
PKG_CONFIG="$orig_pkg_config"

LIBVIRT_SANDBOX_CAPNG
LIBVIRT_SANDBOX_GETTEXT
LIBVIRT_SANDBOX_GTK_MISC
LIBVIRT_SANDBOX_WIN32
LIBVIRT_SANDBOX_COVERAGE
LIBVIRT_SANDBOX_RPCGEN
LIBVIRT_SANDBOX_SELINUX
LIBVIRT_SANDBOX_XDR

LIBVIRT_SANDBOX_STATIC_LIBC

dnl search for LDD path
AC_PATH_PROG([LDD_PATH], [ldd])
if test -z "$LDD_PATH"; then
    AC_MSG_ERROR([Failed to find ldd.])
fi
AC_DEFINE_UNQUOTED([LDD_PATH], "$LDD_PATH", [path to ldd binary])

GOBJECT_INTROSPECTION_CHECK([$GOBJECT_INTROSPECTION_REQUIRED])

dnl Should be in m4/virt-gettext.m4 but intltoolize is too
dnl dumb to find it there
IT_PROG_INTLTOOL([0.35.0])

AM_PATH_PYTHON

AC_OUTPUT(Makefile
          libvirt-sandbox/Makefile
          libvirt-sandbox/tests/Makefile
          libvirt-sandbox/image/Makefile
          libvirt-sandbox/image/sources/Makefile
          bin/Makefile
          examples/Makefile
          docs/Makefile
          docs/libvirt-sandbox/Makefile
          po/Makefile.in
          libvirt-sandbox.spec
          libvirt-sandbox-1.0.pc)

AC_MSG_NOTICE([])
AC_MSG_NOTICE([Configuration summary])
AC_MSG_NOTICE([=====================])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([ Libraries:])
AC_MSG_NOTICE([])
if test "$with_lzma" != "no" ; then
AC_MSG_NOTICE([            LZMA: $LZMA_CFLAGS $LZMA_LIBS])
else
AC_MSG_NOTICE([            LZMA: no])
fi
if test "$with_zlib" != "no" ; then
AC_MSG_NOTICE([            ZLIB: $ZLIB_CFLAGS $ZLIB_LIBS])
else
AC_MSG_NOTICE([            ZLIB: no])
fi
AC_MSG_NOTICE([         GOBJECT: $GOBJECT_CFLAGS $GOBJECT_LIBS])
AC_MSG_NOTICE([ LIBVIRT_GOBJECT: $LIBVIRT_GOBJECT_CFLAGS $LIBVIRT_GOBJECT_LIBS])
AC_MSG_NOTICE([])
