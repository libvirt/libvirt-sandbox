AC_DEFUN([LIBVIRT_SANDBOX_GETTEXT],[
    GETTEXT_PACKAGE=libvirt-sandbox
    AC_SUBST(GETTEXT_PACKAGE)
    AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],"$GETTEXT_PACKAGE", [GETTEXT package name])
])
