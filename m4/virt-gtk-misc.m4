AC_DEFUN([LIBVIRT_SANDBOX_GTK_MISC],[
    GTK_DOC_CHECK([1.10],[--flavour no-tmpl])

    # Setup GLIB_MKENUMS to use glib-mkenums even if GLib is uninstalled.
    GLIB_MKENUMS=`$PKG_CONFIG --variable=glib_mkenums glib-2.0`
    AC_SUBST(GLIB_MKENUMS)
])
