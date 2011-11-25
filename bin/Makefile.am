
bin_PROGRAMS = virt-sandbox

virt_sandbox_SOURCES = virt-sandbox.c
virt_sandbox_CFLAGS = \
		$(GIO_UNIX_CFLAGS) \
		$(LIBVIRT_GOBJECT_CFLAGS) \
		-I$(top_srcdir) \
		$(WARN_CFLAGS) \
		$(NULL)
virt_sandbox_LDFLAGS = \
		$(GIO_UNIX_LIBS) \
		$(LIBVIRT_GOBJECT_LIBS) \
		../libvirt-sandbox/libvirt-sandbox-1.0.la \
		$(WARN_CFLAGS) \
		$(NULL)
