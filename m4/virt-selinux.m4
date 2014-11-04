AC_DEFUN([LIBVIRT_SANDBOX_SELINUX], [
  fail=0
  SELINUX_CFLAGS=
  SELINUX_LIBS=
  old_LIBS=$LIBS
  old_CFLAGS=$CFLAGS
  AC_CHECK_HEADER([selinux/selinux.h],[],[fail=1])
  AC_CHECK_LIB([selinux], [fgetfilecon],[SELINUX_LIBS="$SELINUX_LIBS -lselinux"],[fail=1])
  LIBS=$old_LIBS
  CFLAGS=$old_CFLAGS
  test $fail = 1 &&
    AC_MSG_ERROR([You must install the libselinux development package in order to compile libvirt-sandbox])
  AC_SUBST([SELINUX_CFLAGS])
  AC_SUBST([SELINUX_LIBS])
])
