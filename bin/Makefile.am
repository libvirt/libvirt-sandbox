
bin_PROGRAMS = virt-sandbox

libexec_PROGRAMS = virt-sandbox-service-util

bin_SCRIPTS = virt-sandbox-service

virtsandboxcompdir = $(datarootdir)/bash-completion/completions/

crondailydir = $(sysconfdir)/cron.daily
crondaily_SCRIPTS = virt-sandbox-service.logrotate

POD_FILES = \
	virt-sandbox-service.pod \
	virt-sandbox-service-execute.pod \
	virt-sandbox-service-create.pod \
	virt-sandbox-service-clone.pod \
	virt-sandbox-service-connect.pod \
	virt-sandbox-service-delete.pod \
	virt-sandbox-service-reload.pod \
	virt-sandbox-service-upgrade.pod \
	$(NULL)
EXTRA_DIST = $(bin_SCRIPTS) $(POD_FILES) virt-sandbox-service-bash-completion.sh virt-sandbox-service.logrotate
EXTRA_DIST += virt-sandbox-service-bash-completion.sh

man1_MANS = \
	virt-sandbox.1 \
	virt-sandbox-service.1 \
	virt-sandbox-service-execute.1 \
	virt-sandbox-service-create.1 \
	virt-sandbox-service-clone.1 \
	virt-sandbox-service-connect.1 \
	virt-sandbox-service-delete.1 \
	virt-sandbox-service-reload.1 \
	virt-sandbox-service-upgrade.1 \
	$(NULL)

POD2MAN = pod2man -c "Virtualization Support" -r "$(PACKAGE)-$(VERSION)"

virt-sandbox.1: virt-sandbox.c Makefile
	$(AM_V_GEN)$(POD2MAN) $< $(srcdir)/$@

virt-sandbox-service.1: virt-sandbox-service.pod Makefile
	$(AM_V_GEN)$(POD2MAN) $< $(srcdir)/$@

virt-sandbox-service-execute.1: virt-sandbox-service-execute.pod Makefile
	$(AM_V_GEN)$(POD2MAN) $< $(srcdir)/$@

virt-sandbox-service-clone.1: virt-sandbox-service-clone.pod Makefile
	$(AM_V_GEN)$(POD2MAN) $< $(srcdir)/$@

virt-sandbox-service-create.1: virt-sandbox-service-create.pod Makefile
	$(AM_V_GEN)$(POD2MAN) $< $(srcdir)/$@

virt-sandbox-service-connect.1: virt-sandbox-service-connect.pod Makefile
	$(AM_V_GEN)$(POD2MAN) $< $(srcdir)/$@

virt-sandbox-service-delete.1: virt-sandbox-service-delete.pod Makefile
	$(AM_V_GEN)$(POD2MAN) $< $(srcdir)/$@

virt-sandbox-service-reload.1: virt-sandbox-service-reload.pod Makefile
	$(AM_V_GEN)$(POD2MAN) $< $(srcdir)/$@

virt-sandbox-service-upgrade.1: virt-sandbox-service-upgrade.pod Makefile
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

virt_sandbox_service_util_SOURCES = virt-sandbox-service-util.c
virt_sandbox_service_util_CFLAGS = \
		$(GIO_UNIX_CFLAGS) \
		$(LIBVIRT_GOBJECT_CFLAGS) \
		-I$(top_srcdir) \
		-DLOCALEDIR="\"$(datadir)/locale"\" \
		$(WARN_CFLAGS) \
		$(NULL)
virt_sandbox_service_util_LDFLAGS = \
		$(GIO_UNIX_LIBS) \
		$(LIBVIRT_GOBJECT_LIBS) \
		../libvirt-sandbox/libvirt-sandbox-1.0.la \
		$(WARN_CFLAGS) \
		$(NULL)

install-data-local:
		$(MKDIR_P) $(DESTDIR)$(sysconfdir)/libvirt-sandbox/services
		$(MKDIR_P) $(DESTDIR)$(virtsandboxcompdir)
		cp $(srcdir)/virt-sandbox-service-bash-completion.sh $(DESTDIR)$(virtsandboxcompdir)/virt-sandbox-service

uninstall-local:
		$(rmdir) $(DESTDIR)$(sysconfdir)/libvirt-sandbox/services ||:
		$(rmdir) $(DESTDIR)$(sysconfdir)/libvirt-sandbox ||:
		rm -f $(DESTDIR)$(virtsandboxcompdir)/virt-sandbox-service
