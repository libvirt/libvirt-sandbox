AC_DEFUN([LIBVIRT_SANDBOX_XDR], [
    old_LIBS="$LIBS"
    AC_SEARCH_LIBS([xdrmem_create], [portablexdr rpc xdr nsl tirpc], [
      XDR_LIBS="$ac_cv_search_xdrmem_create"
    ],[
      AC_MSG_ERROR([Cannot find a XDR library])
    ])
    LIBS="$old_LIBS"

    AC_CACHE_CHECK([where to find <rpc/rpc.h>], [lv_cv_xdr_cflags], [
      for add_CFLAGS in '' '-I/usr/include/tirpc' 'missing'; do
        if test x"$add_CFLAGS" = xmissing; then
          lv_cv_xdr_cflags=missing; break
        fi
        CFLAGS="$old_CFLAGS $add_CFLAGS"
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <rpc/rpc.h>
        ]])], [lv_cv_xdr_cflags=${add_CFLAGS:-none}; break])
      done
    ])
    CFLAGS=$old_CFLAGS
    case $lv_cv_xdr_cflags in
      none) XDR_CFLAGS= ;;
      missing) AC_MSG_ERROR([Unable to find <rpc/rpc.h>]) ;;
      *) XDR_CFLAGS=$lv_cv_xdr_cflags ;;
    esac

    AC_SUBST([XDR_LIBS])
    AC_SUBST([XDR_CFLAGS])
])
