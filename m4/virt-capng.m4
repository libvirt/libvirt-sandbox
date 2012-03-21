AC_DEFUN([LIBVIRT_SANDBOX_CAPNG], [
    dnl libcap-ng
    AC_ARG_WITH([capng],
      AC_HELP_STRING([--with-capng], [use libcap-ng to reduce sandbox privileges @<:@default=check@:>@]),
        [],
        [with_capng=check])

    dnl
    dnl This check looks for 'capng_updatev' since that was
    dnl introduced in 0.4.0 release which need as minimum
    dnl
    CAPNG_CFLAGS=
    CAPNG_LIBS=
    if test "$with_capng" != "no"; then
      old_cflags="$CFLAGS"
      old_libs="$LIBS"
      if test "$with_capng" = "check"; then
        AC_CHECK_HEADER([cap-ng.h],[],[with_capng=no])
        AC_CHECK_LIB([cap-ng], [capng_updatev],[],[with_capng=no])
        if test "$with_capng" != "no"; then
          with_capng="yes"
        fi
      else
        fail=0
        AC_CHECK_HEADER([cap-ng.h],[],[fail=1])
        AC_CHECK_LIB([cap-ng], [capng_updatev],[],[fail=1])
        test $fail = 1 &&
          AC_MSG_ERROR([You must install the capng >= 0.4.0 development package in order to compile and run virt-sandbox])
      fi
      CFLAGS="$old_cflags"
      LIBS="$old_libs"
    fi
    if test "$with_capng" = "yes"; then
      CAPNG_LIBS="-lcap-ng"
      AC_DEFINE_UNQUOTED([HAVE_CAPNG], 1, [whether capng is available for privilege reduction])
    fi
    AM_CONDITIONAL([HAVE_CAPNG], [test "$with_capng" != "no"])
    AC_SUBST([CAPNG_CFLAGS])
    AC_SUBST([CAPNG_LIBS])
])
