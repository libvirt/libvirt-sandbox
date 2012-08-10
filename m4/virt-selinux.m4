AC_DEFUN([LIBVIRT_SANDBOX_SELINUX], [
  fail=0
  old_LIBS=$LIBS
  old_CFLAGS=$CFLAGS
  AC_CHECK_HEADER([selinux/selinux.h],[],[fail=1])
  AC_CHECK_LIB([selinux], [fgetfilecon],[],[fail=1])
  LIBS=$old_LIBS
  CFLAGS=$old_CFLAGS
  test $fail = 1 &&
    AC_MSG_ERROR([You must install the libselinux development package in order to compile libvirt-sandbox])
])
