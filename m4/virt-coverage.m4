AC_DEFUN([LIBVIRT_SANDBOX_COVERAGE],[
    AC_ARG_ENABLE([test-coverage],
      [  --enable-test-coverage  turn on code coverage instrumentation],
      [case "${enableval}" in
         yes|no) ;;
              *)      AC_MSG_ERROR([bad value ${enableval} for test-coverage option]) ;;
       esac],
              [enableval=no])
    enable_coverage=$enableval

    if test "${enable_coverage}" = yes; then
      save_WARN_CFLAGS=$WARN_CFLAGS
      WARN_CFLAGS=
      gl_WARN_ADD([-fprofile-arcs])
      gl_WARN_ADD([-ftest-coverage])
      COVERAGE_FLAGS=$WARN_CFLAGS
      AC_SUBST([COVERAGE_CFLAGS], [$COVERAGE_FLAGS])
      AC_SUBST([COVERAGE_LDFLAGS], [$COVERAGE_FLAGS])
      WARN_CFLAGS=$save_WARN_CFLAGS
    fi
])
