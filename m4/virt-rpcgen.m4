AC_DEFUN([LIBVIRT_SANDBOX_RPCGEN], [
    AC_PATH_PROG([PERL], [perl], [no])
    AC_PATH_PROG([RPCGEN], [rpcgen], [no])

    if test "$ac_cv_path_PERL" = "no" ; then
      AC_MSG_ERROR([perl is required to build the libvirt-sandbox])
    fi
    if test "$ac_cv_path_RPCGEN" = "no" ; then
      AC_MSG_ERROR([rpcgen is required to build the libvirt-sandbox])
    fi
])
