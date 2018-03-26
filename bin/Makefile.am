
bin_PROGRAMS = virt-sandbox

bin_SCRIPTS = virt-sandbox-image

EXTRA_DIST = $(bin_SCRIPTS)

man1_MANS = \
	virt-sandbox.1 \
	$(NULL)

POD2MAN = pod2man -c "Virtualization Support" -r "$(PACKAGE)-$(VERSION)"

virt-sandbox.1: virt-sandbox.c Makefile
	$(AM_V_GEN)$(POD2MAN) $< $(srcdir)/$@

CLEANFILES = $(man1_MANS)

virt_sandbox_SOURCES = virt-sandbox.c
virt_sandbox_CFLAGS = \
		$(GIO_UNIX_CFLAGS) \
		$(LIBVIRT_GOBJECT_CFLAGS) \
		-I$(top_srcdir) \
		-DLOCALEDIR="\"$(datadir)/locale"\" \
		$(WARN_CFLAGS) \
		$(NULL)
virt_sandbox_LDFLAGS = \
		$(GIO_UNIX_LIBS) \
		$(LIBVIRT_GOBJECT_LIBS) \
		../libvirt-sandbox/libvirt-sandbox-1.0.la \
		$(WARN_CFLAGS) \
		$(NULL)
