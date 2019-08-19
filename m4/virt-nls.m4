dnl gettext utilities
dnl
dnl Copyright (C) 2018 Red Hat, Inc.
dnl
dnl This library is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU Lesser General Public
dnl License as published by the Free Software Foundation; either
dnl version 2.1 of the License, or (at your option) any later version.
dnl
dnl This library is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl Lesser General Public License for more details.
dnl
dnl You should have received a copy of the GNU Lesser General Public
dnl License along with this library.  If not, see
dnl <http://www.gnu.org/licenses/>.
dnl

AC_DEFUN([LIBVIRT_SANDBOX_ARG_NLS],[
  m4_divert_text([DEFAULTS], [[enable_nls=yes]])
  AC_ARG_ENABLE([nls],
                [AS_HELP_STRING([--enable-nls],
                                [NLS @<:@default=yes@:>@])])
])

AC_DEFUN([LIBVIRT_SANDBOX_CHECK_NLS],[
  dnl GNU gettext tools (optional).
  AC_CHECK_PROG([XGETTEXT], [xgettext], [xgettext], [no])
  AC_CHECK_PROG([MSGFMT], [msgfmt], [msgfmt], [no])
  AC_CHECK_PROG([MSGMERGE], [msgmerge], [msgmerge], [no])

  dnl Check they are the GNU gettext tools.
  AC_MSG_CHECKING([msgfmt is GNU tool])
  if $MSGFMT --version >/dev/null 2>&1 && $MSGFMT --version | grep -q 'GNU gettext'; then
    msgfmt_is_gnu=yes
  else
    msgfmt_is_gnu=no
  fi
  AC_MSG_RESULT([$msgfmt_is_gnu])
  AM_CONDITIONAL([ENABLE_NLS], [test "x$enable_nls" = "xyes"])
  AM_CONDITIONAL([HAVE_GNU_GETTEXT_TOOLS],
    [test "x$XGETTEXT" != "xno" && test "x$MSGFMT" != "xno" && \
     test "x$MSGMERGE" != "xno" && test "x$msgfmt_is_gnu" != "xno"])
])
