
bin_PROGRAMS = virt-sandbox

libexec_PROGRAMS = virt-sandbox-service-util

bin_SCRIPTS = virt-sandbox-service

virtsandboxcompdir = $(sysconfdir)/bash_completion.d/
virtsandboxcomp_DATA = virt-sandbox-service-bash-completion.sh

crondailydir = $(sysconfdir)/cron.daily
crondaily_SCRIPTS = virt-sandbox-service.logrotate

POD_FILES = virt-sandbox-service.pod virt-sandbox-service-execute.pod virt-sandbox-service-create.pod virt-sandbox-service-clone.pod virt-sandbox-service-connect.pod virt-sandbox-service-delete.pod virt-sandbox-service-start.pod virt-sandbox-service-stop.pod virt-sandbox-service-reload.pod virt-sandbox-service-list.pod
EXTRA_DIST = $(bin_SCRIPTS) $(POD_FILES) virt-sandbox-service-bash-completion.sh virt-sandbox-service.logrotate

man1_MANS = virt-sandbox.1 virt-sandbox-service.1

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

virt-sandbox-service-start.1: virt-sandbox-service-start.pod Makefile
        $(AM_V_GEN)$(POD2MAN) $< $(srcdir)/$@

virt-sandbox-service-stop.1: virt-sandbox-service-stop.pod Makefile
        $(AM_V_GEN)$(POD2MAN) $< $(srcdir)/$@

virt-sandbox-service-connect.1: virt-sandbox-service-connect.pod Makefile
        $(AM_V_GEN)$(POD2MAN) $< $(srcdir)/$@

virt-sandbox-service-delete.1: virt-sandbox-service-delete.pod Makefile
        $(AM_V_GEN)$(POD2MAN) $< $(srcdir)/$@

virt-sandbox-service-reload.1: virt-sandbox-service-reload.pod Makefile
        $(AM_V_GEN)$(POD2MAN) $< $(srcdir)/$@

virt-sandbox-service-list.1: virt-sandbox-service-list.pod Makefile
        $(AM_V_GEN)$(POD2MAN) $< $(srcdir)/$@

CLEANFILES = $(man1_MANS)

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

virt_sandbox_service_util_SOURCES = virt-sandbox-service-util.c  container.c
virt_sandbox_service_util_CFLAGS = \
                $(GIO_UNIX_CFLAGS) \
                $(LIBVIRT_GOBJECT_CFLAGS) \
                -I$(top_srcdir) \
                $(WARN_CFLAGS) \
                $(NULL)
virt_sandbox_service_util_LDFLAGS = \
                $(GIO_UNIX_LIBS) \
                $(LIBVIRT_GOBJECT_LIBS) \
                ../libvirt-sandbox/libvirt-sandbox-1.0.la \
                -lselinux \
                $(WARN_CFLAGS) \
                $(NULL)

install-data-local:
                $(MKDIR_P) $(DESTDIR)$(sysconfdir)/libvirt-sandbox/services

uninstall-local:
                $(rmdir) $(DESTDIR)$(sysconfdir)/libvirt-sandbox/services ||:
                $(rmdir) $(DESTDIR)$(sysconfdir)/libvirt-sandbox ||:
