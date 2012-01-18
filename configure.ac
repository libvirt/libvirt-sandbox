AC_INIT(libvirt-sandbox, 0.0.2)
AC_CONFIG_SRCDIR(libvirt-sandbox/libvirt-sandbox.h)
AM_CONFIG_HEADER(config.h)
dnl Make automake keep quiet about wildcards & other GNUmake-isms
AM_INIT_AUTOMAKE([-Wno-portability])
AC_CANONICAL_HOST

AM_SILENT_RULES([yes])

GIO_UNIX_REQUIRED=2.28.0
GOBJECT_REQUIRED=2.28.0
LIBVIRT_GLIB_REQUIRED=0.0.4
LIBVIRT_GOBJECT_REQUIRED=0.0.4
GOBJECT_INTROSPECTION_REQUIRED=0.10.8

LIBVIRT_SANDBOX_MAJOR_VERSION=`echo $VERSION | awk -F. '{print $1}'`
LIBVIRT_SANDBOX_MINOR_VERSION=`echo $VERSION | awk -F. '{print $2}'`
LIBVIRT_SANDBOX_MICRO_VERSION=`echo $VERSION | awk -F. '{print $3}'`
LIBVIRT_SANDBOX_VERSION=$LIBVIRT_SANDBOX_MAJOR_VERSION.$LIBVIRT_SANDBOX_MINOR_VERSION.$LIBVIRT_SANDBOX_MICRO_VERSION$LIBVIRT_SANDBOX_MICRO_VERSION_SUFFIX
LIBVIRT_SANDBOX_VERSION_INFO=`expr $LIBVIRT_SANDBOX_MAJOR_VERSION + $LIBVIRT_SANDBOX_MINOR_VERSION`:$LIBVIRT_SANDBOX_MICRO_VERSION:$LIBVIRT_SANDBOX_MINOR_VERSION
LIBVIRT_SANDBOX_VERSION_NUMBER=`expr $LIBVIRT_SANDBOX_MAJOR_VERSION \* 1000000 + $LIBVIRT_SANDBOX_MINOR_VERSION \* 1000 + $LIBVIRT_SANDBOX_MICRO_VERSION`

AC_SUBST([LIBVIRT_SANDBOX_MAJOR_VERSION])
AC_SUBST([LIBVIRT_SANDBOX_MINOR_VERSION])
AC_SUBST([LIBVIRT_SANDBOX_MICRO_VERSION])
AC_SUBST([LIBVIRT_SANDBOX_VERSION])
AC_SUBST([LIBVIRT_SANDBOX_VERSION_INFO])
AC_SUBST([LIBVIRT_SANDBOX_VERSION_NUMBER])

AC_PROG_CC
AM_PROG_CC_C_O

AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

AC_CONFIG_LIBOBJ_DIR([libvirt-sandbox])

LIBVIRT_SANDBOX_COMPILE_WARNINGS(maximum)

# Autoconf 2.61a.99 and earlier don't support linking a file only
# in VPATH builds.  But since GNUmakefile is for maintainer use
# only, it does not matter if we skip the link with older autoconf.
# Automake 1.10.1 and earlier try to remove GNUmakefile in non-VPATH
# builds, so use a shell variable to bypass this.
GNUmakefile=GNUmakefile
m4_if(m4_version_compare([2.61a.100],
        m4_defn([m4_PACKAGE_VERSION])), [1], [],
      [AC_CONFIG_LINKS([$GNUmakefile:$GNUmakefile], [],
        [GNUmakefile=$GNUmakefile])])

GETTEXT_PACKAGE=libvirt-sandbox
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],"$GETTEXT_PACKAGE", [GETTEXT package name])

dnl IT_PROG_INTLTOOL([0.35.0])

PKG_CHECK_MODULES(GIO_UNIX, gio-unix-2.0 >= $GIO_UNIX_REQUIRED)
PKG_CHECK_MODULES(GOBJECT, gobject-2.0 >= $GOBJECT_REQUIRED)
PKG_CHECK_MODULES(LIBVIRT_GLIB, libvirt-glib-1.0 >= $LIBVIRT_GOBJECT_REQUIRED)
PKG_CHECK_MODULES(LIBVIRT_GOBJECT, libvirt-gobject-1.0 >= $LIBVIRT_GOBJECT_REQUIRED)

GTK_DOC_CHECK([1.10],[--flavour no-tmpl])

# Setup GLIB_MKENUMS to use glib-mkenums even if GLib is uninstalled.
GLIB_MKENUMS=`$PKG_CONFIG --variable=glib_mkenums glib-2.0`
AC_SUBST(GLIB_MKENUMS)

dnl Extra link-time flags for Cygwin.
dnl Copied from libxml2 configure.in, but I removed mingw changes
dnl for now since I'm not supporting mingw at present.  - RWMJ
CYGWIN_EXTRA_LDFLAGS=
CYGWIN_EXTRA_LIBADD=
CYGWIN_EXTRA_PYTHON_LIBADD=
MINGW_EXTRA_LDFLAGS=
case "$host" in
  *-*-cygwin*)
    CYGWIN_EXTRA_LDFLAGS="-no-undefined"
    CYGWIN_EXTRA_LIBADD="${INTLLIBS}"
    if test "x$PYTHON_VERSION" != "x"; then
      CYGWIN_EXTRA_PYTHON_LIBADD="-L/usr/lib/python${PYTHON_VERSION}/config -lpython${PYTHON_VERSION}"
    fi
    ;;
  *-*-mingw*)
    MINGW_EXTRA_LDFLAGS="-no-undefined"
    ;;
esac
AC_SUBST([CYGWIN_EXTRA_LDFLAGS])
AC_SUBST([CYGWIN_EXTRA_LIBADD])
AC_SUBST([CYGWIN_EXTRA_PYTHON_LIBADD])
AC_SUBST([MINGW_EXTRA_LDFLAGS])


AC_ARG_ENABLE([test-coverage],
[  --enable-test-coverage  turn on code coverage instrumentation],
[case "${enableval}" in
   yes|no) ;;
   *)      AC_MSG_ERROR([bad value ${enableval} for test-coverage option]) ;;
 esac],
              [enableval=no])
enable_coverage=$enableval

if test "${enable_coverage}" = yes; then
  gl_COMPILER_FLAGS(-fprofile-arcs)
  gl_COMPILER_FLAGS(-ftest-coverage)
  AC_SUBST([COVERAGE_CFLAGS], [$COMPILER_FLAGS])
  AC_SUBST([COVERAGE_LDFLAGS], [$COMPILER_FLAGS])
  COMPILER_FLAGS=
fi

AC_ARG_ENABLE([introspection],
        AS_HELP_STRING([--enable-introspection], [enable GObject introspection]),
        [], [enable_introspection=check])

if test "x$enable_introspection" != "xno" ; then
        PKG_CHECK_MODULES([GOBJECT_INTROSPECTION],
                          [gobject-introspection-1.0 >= $GOBJECT_INTROSPECTION_REQUIRED],
                          [enable_introspection=yes],
                          [
                             if test "x$enable_introspection" = "xcheck"; then
                               enable_introspection=no
                             else
                               AC_MSG_ERROR([gobject-introspection is not available])
                             fi
                          ])
        if test "x$enable_introspection" = "xyes" ; then
          AC_DEFINE([WITH_GOBJECT_INTROSPECTION], [1], [enable GObject introspection support])
          AC_SUBST(GOBJECT_INTROSPECTION_CFLAGS)
          AC_SUBST(GOBJECT_INTROSPECTION_LIBS)
          AC_SUBST([G_IR_SCANNER], [$($PKG_CONFIG --variable=g_ir_scanner gobject-introspection-1.0)])
          AC_SUBST([G_IR_COMPILER], [$($PKG_CONFIG --variable=g_ir_compiler gobject-introspection-1.0)])
        fi
fi
AM_CONDITIONAL([WITH_INTROSPECTION], [test "x$enable_introspection" = "xyes"])


AC_OUTPUT(Makefile
          libvirt-sandbox/Makefile
          libvirt-sandbox/tests/Makefile
          bin/Makefile
          examples/Makefile
          docs/Makefile
          docs/libvirt-sandbox/Makefile
          libvirt-sandbox.spec
          libvirt-sandbox-1.0.pc)

AC_MSG_NOTICE([])
AC_MSG_NOTICE([Configuration summary])
AC_MSG_NOTICE([=====================])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([ Libraries:])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([         GOBJECT: $GOBJECT_CFLAGS $GOBJECT_LIBS])
AC_MSG_NOTICE([ LIBVIRT_GOBJECT: $LIBVIRT_GOBJECT_CFLAGS $LIBVIRT_GOBJECT_LIBS])
AC_MSG_NOTICE([])
