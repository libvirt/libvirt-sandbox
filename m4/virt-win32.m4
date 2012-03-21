AC_DEFUN([LIBVIRT_SANDBOX_WIN32],[
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
])
