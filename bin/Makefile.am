
bin_PROGRAMS = virt-sandbox

bin_SCRIPTS = virt-sandbox-service

virtsandboxcompdir = $(sysconfdir)/bash_completion.d/
virtsandboxcomp_DATA = virt-sandbox-service-bash-completion.sh

EXTRA_DIST = $(bin_SCRIPTS) virt-sandbox-service-bash-completion.sh virt-sandbox-service.1

man1_MANS = virt-sandbox.1 virt-sandbox-service.1

POD2MAN = pod2man -c "Virtualization Support" -r "$(PACKAGE)-$(VERSION)"

virt-sandbox.1: virt-sandbox.c Makefile
	$(AM_V_GEN)$(POD2MAN) $< $(srcdir)/$@

CLEANFILES = virt-sandbox.1

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
