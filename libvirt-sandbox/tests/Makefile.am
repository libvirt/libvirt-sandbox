

TESTS = test-config

check_PROGRAMS = test-config

test_config_SOURCES = test-config.c
test_config_LDADD = \
			../libvirt-sandbox-1.0.la \
			$(GIO_UNIX_LIBS) \
			$(LIBVIRT_GLIB_LIBS) \
			$(LIBVIRT_GOBJECT_LIBS) \
			$(CYGWIN_EXTRA_LIBADD)
test_config_CFLAGS = \
			$(COVERAGE_CFLAGS) \
			-I$(top_srcdir) \
			$(GIO_UNIX_CFLAGS) \
			$(LIBVIRT_GLIB_CFLAGS) \
			$(LIBVIRT_GOBJECT_CFLAGS) \
			$(WARN_CFLAGS)
