# LIBVIRT_SANDBOX_STATIC_LIB(NAME, CFLAGS, LDFLAGS, PROLOG, PROGRAM)
# ------------------------------------------------------------------
# Check if the program can be linked with static libraries only.
#
AC_DEFUN([LIBVIRT_SANDBOX_STATIC_LIB], [
    AC_MSG_CHECKING([for static $1])

    SAVED_LDFLAGS=$LDFLAGS
    SAVED_CFLAGS=$CFLAGS
    CFLAGS="$2"
    LDFLAGS="-static $3"
    AC_LINK_IFELSE(
        [AC_LANG_PROGRAM([$4],
                         [$5])],
        [],
        [AC_MSG_RESULT([no])]
        [AC_MSG_ERROR([static $1 is required in order to build virt-sandbox-init-qemu])]
    )
    LDFLAGS=$SAVED_LDFLAGS
    CFLAGS=$SAVED_CFLAGS

    AC_MSG_RESULT([yes])
])

AC_DEFUN([LIBVIRT_SANDBOX_STATIC_LIBC], [
    LIBVIRT_SANDBOX_STATIC_LIB(
         [LIBC],
         [],
         [],
         [#include <stdio.h>],
         [printf("bar");])
])
