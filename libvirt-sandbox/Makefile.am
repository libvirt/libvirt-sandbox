
EXTRA_DIST = libvirt-sandbox.sym
CLEANFILES =

SUBDIRS = tests

rundir = $(localstatedir)/run

libexec_PROGRAMS = \
	libvirt-sandbox-init-common \
	libvirt-sandbox-init-qemu \
	libvirt-sandbox-init-lxc \
	$(NULL)

PROTOCOL_GENERATOR = libvirt-sandbox-protocol-generator.pl

EXTRA_DIST += $(PROTOCOL_GENERATOR)

sandboxconfigdir = $(sysconfdir)/$(PACKAGE)/scratch
sandboxconfig_DATA = README

lib_LTLIBRARIES = libvirt-sandbox-1.0.la

SANDBOX_TEMPLATE_FILES = \
			libvirt-sandbox-enum-types.h.template \
			libvirt-sandbox-enum-types.c.template \
			$(NULL)

SANDBOX_GENERATED_ENUM_FILES = \
			libvirt-sandbox-enum-types.h \
			libvirt-sandbox-enum-types.c \
			$(NULL)

SANDBOX_GENERATED_RPC_FILES = \
			libvirt-sandbox-protocol.h \
			libvirt-sandbox-protocol.c \
			$(NULL)

SANDBOX_PROTOCOL_FILES = \
			libvirt-sandbox-protocol.x \
			$(NULL)

SANDBOX_GENERATED_FILES = \
			$(SANDBOX_GENERATED_ENUM_FILES) \
			$(SANDBOX_GENERATED_RPC_FILES) \
			$(NULL)

SANDBOX_RPC_FILES = \
			libvirt-sandbox-rpcpacket.c \
			libvirt-sandbox-rpcpacket.h \
			$(NULL)

SANDBOX_HEADER_FILES = \
			libvirt-sandbox.h \
			libvirt-sandbox-main.h \
			libvirt-sandbox-util.h \
			libvirt-sandbox-config.h \
			libvirt-sandbox-config-network.h \
			libvirt-sandbox-config-network-address.h \
			libvirt-sandbox-config-network-route.h \
			libvirt-sandbox-config-mount.h \
			libvirt-sandbox-config-mount-file.h \
			libvirt-sandbox-config-mount-host-bind.h \
			libvirt-sandbox-config-mount-host-image.h \
			libvirt-sandbox-config-mount-guest-bind.h \
			libvirt-sandbox-config-mount-ram.h \
			libvirt-sandbox-config-initrd.h \
			libvirt-sandbox-config-interactive.h \
			libvirt-sandbox-config-service.h \
			libvirt-sandbox-config-service-systemd.h \
			libvirt-sandbox-config-service-generic.h \
			libvirt-sandbox-builder.h \
			libvirt-sandbox-builder-initrd.h \
			libvirt-sandbox-builder-machine.h \
			libvirt-sandbox-builder-container.h \
			libvirt-sandbox-console.h \
			libvirt-sandbox-console-raw.h \
			libvirt-sandbox-console-rpc.h \
			libvirt-sandbox-context.h \
			libvirt-sandbox-context-interactive.h \
			libvirt-sandbox-context-service.h \
			$(NULL)
SANDBOX_SOURCE_FILES = \
			libvirt-sandbox-main.c \
			libvirt-sandbox-config.c \
			libvirt-sandbox-config-network.c \
			libvirt-sandbox-config-network-address.c \
			libvirt-sandbox-config-network-route.c \
			libvirt-sandbox-config-mount.c \
			libvirt-sandbox-config-mount-file.c \
			libvirt-sandbox-config-mount-host-bind.c \
			libvirt-sandbox-config-mount-host-image.c \
			libvirt-sandbox-config-mount-guest-bind.c \
			libvirt-sandbox-config-mount-ram.c \
			libvirt-sandbox-config-initrd.c \
			libvirt-sandbox-config-interactive.c \
			libvirt-sandbox-config-service.c \
			libvirt-sandbox-config-service-systemd.c \
			libvirt-sandbox-config-service-generic.c \
			libvirt-sandbox-builder.c \
			libvirt-sandbox-builder-initrd.c \
			libvirt-sandbox-builder-machine.c \
			libvirt-sandbox-builder-container.c \
			libvirt-sandbox-console.c \
			libvirt-sandbox-console-raw.c \
			libvirt-sandbox-console-rpc.c \
			libvirt-sandbox-context.c \
			libvirt-sandbox-context-interactive.c \
			libvirt-sandbox-context-service.c \
			$(NULL)

libvirt_sandbox_1_0_ladir = $(includedir)/libvirt-sandbox-1.0/libvirt-sandbox
libvirt_sandbox_1_0_la_HEADERS = \
			$(SANDBOX_HEADER_FILES)
libvirt_sandbox_1_0_la_SOURCES = \
			$(SANDBOX_SOURCE_FILES) \
			$(SANDBOX_RPC_FILES)
nodist_libvirt_sandbox_1_0_la_HEADERS = \
			libvirt-sandbox-enum-types.h
			$(NULL)
nodist_libvirt_sandbox_1_0_la_SOURCES = \
			libvirt-sandbox-enum-types.c \
			libvirt-sandbox-protocol.h \
			libvirt-sandbox-protocol.c \
			$(NULL)
libvirt_sandbox_1_0_la_CFLAGS = \
			-DBINDIR="\"$(bindir)\"" \
			-DDATADIR="\"$(datadir)\"" \
			-DLIBVIRT_SANDBOX_BUILD \
			-DLIBEXECDIR="\"$(libexecdir)\"" \
			-DSANDBOXCONFIGDIR="\"$(sandboxconfigdir)\"" \
			-DRUNDIR="\"$(rundir)\"" \
			-DSYSCONFDIR="\"$(sysconfdir)\"" \
			-DLOCALEDIR="\"$(datadir)/locale"\" \
			$(COVERAGE_CFLAGS) \
			-I$(top_srcdir) \
			$(GIO_UNIX_CFLAGS) \
			$(LIBVIRT_GLIB_CFLAGS) \
			$(LIBVIRT_GOBJECT_CFLAGS) \
			$(WARN_CFLAGS) \
			$(NULL)
libvirt_sandbox_1_0_la_LIBADD = \
			$(GIO_UNIX_LIBS) \
			$(LIBVIRT_GLIB_LIBS) \
			$(LIBVIRT_GOBJECT_LIBS) \
			$(CYGWIN_EXTRA_LIBADD) \
			$(NULL)
libvirt_sandbox_1_0_la_DEPENDENCIES = \
                        libvirt-sandbox.sym
libvirt_sandbox_1_0_la_LDFLAGS = \
			$(WARN_CFLAGS) \
			$(COVERAGE_CFLAGS:-f%=-Wc,f%) \
			$(CYGWIN_EXTRA_LDFLAGS) \
			$(MINGW_EXTRA_LDFLAGS) \
			-Wl,--version-script=$(srcdir)/libvirt-sandbox.sym \
			-version-info $(LIBVIRT_SANDBOX_VERSION_INFO)

libvirt_sandbox_init_common_SOURCES = libvirt-sandbox-init-common.c \
			$(SANDBOX_GENERATED_RPC_FILES) \
			$(SANDBOX_RPC_FILES) \
			$(NULL)
libvirt_sandbox_init_common_CFLAGS = \
			-DLIBEXECDIR="\"$(libexecdir)\"" \
			-DSANDBOXCONFIGDIR="\"$(sandboxconfigdir)\"" \
			-DLOCALEDIR="\"$(datadir)/locale"\" \
			$(COVERAGE_CFLAGS) \
			-I$(top_srcdir) \
			$(GIO_UNIX_CFLAGS) \
			$(LIBVIRT_GLIB_CFLAGS) \
			$(LIBVIRT_GOBJECT_CFLAGS) \
			$(CAPNG_CFLAGS) \
			$(WARN_CFLAGS) \
			$(NULL)
libvirt_sandbox_init_common_LDFLAGS = \
			-lutil \
			$(COVERAGE_CFLAGS:-f%=-Wc,f%) \
			$(GIO_UNIX_LIBS) \
			$(LIBVIRT_GLIB_LIBS) \
			$(LIBVIRT_GOBJECT_LIBS) \
			$(CAPNG_LIBS) \
			$(WARN_CFLAGS) \
			$(NULL)
libvirt_sandbox_init_common_LDADD = \
			libvirt-sandbox-1.0.la \
			$(NULL)

libvirt_sandbox_init_lxc_SOURCES = libvirt-sandbox-init-lxc.c
libvirt_sandbox_init_lxc_CFLAGS = \
			-DLIBEXECDIR="\"$(libexecdir)\"" \
			-DSANDBOXCONFIGDIR="\"$(sandboxconfigdir)\"" \
			-I$(top_srcdir) \
			$(GIO_UNIX_CFLAGS) \
			$(LIBVIRT_GLIB_CFLAGS) \
			$(LIBVIRT_GOBJECT_CFLAGS) \
			$(WARN_CFLAGS) \
			$(NULL)
libvirt_sandbox_init_lxc_LDFLAGS = \
			libvirt-sandbox-1.0.la \
			$(COVERAGE_CFLAGS:-f%=-Wc,f%) \
			$(GIO_UNIX_LIBS) \
			$(LIBVIRT_GLIB_LIBS) \
			$(LIBVIRT_GOBJECT_LIBS) \
			$(WARN_CFLAGS) \
			$(NULL)
libvirt_sandbox_init_lxc_LDADD = \
			libvirt-sandbox-1.0.la \
			$(NULL)

libvirt_sandbox_init_qemu_SOURCES = libvirt-sandbox-init-qemu.c
libvirt_sandbox_init_qemu_CFLAGS = \
			-DLIBEXECDIR="\"$(libexecdir)\"" \
			-DSANDBOXCONFIGDIR="\"$(sandboxconfigdir)\"" \
			$(WARN_CFLAGS) \
			$(NULL)
libvirt_sandbox_init_qemu_LDFLAGS = \
			-all-static \
			$(COVERAGE_CFLAGS:-f%=-Wc,f%) \
			$(WARN_CFLAGS) \
			$(NULL)


EXTRA_DIST += $(SANDBOX_TEMPLATE_FILES) $(SANDBOX_PROTOCOL_FILES)

BUILT_SOURCES = $(SANDBOX_GENERATED_FILES)
CLEANFILES += $(SANDBOX_GENERATED_FILES)

libvirt-sandbox-enum-types.h: $(SANDBOX_HEADER_FILES) libvirt-sandbox-enum-types.h.template
	$(AM_V_GEN) ( $(GLIB_MKENUMS) --template $(srcdir)/libvirt-sandbox-enum-types.h.template $(SANDBOX_HEADER_FILES:%=$(srcdir)/%) ) | \
            sed -e "s/G_TYPE_VIR_CONFIG/GVIR_CONFIG_TYPE/" -e "s/g_vir/gvir/" > libvirt-sandbox-enum-types.h

libvirt-sandbox-enum-types.c: $(SANDBOX_HEADER_FILES) libvirt-sandbox-enum-types.c.template
	$(AM_V_GEN) ( $(GLIB_MKENUMS) --template $(srcdir)/libvirt-sandbox-enum-types.c.template $(SANDBOX_HEADER_FILES:%=$(srcdir)/%) ) | \
            sed -e "s/G_TYPE_VIR_CONFIG/GVIR_CONFIG_TYPE/" -e "s/g_vir/gvir/" > libvirt-sandbox-enum-types.c


libvirt-sandbox-protocol.c: $(srcdir)/$(PROTOCOL_GENERATOR) $(SANDBOX_PROTOCOL_FILES)
	$(AM_V_GEN)$(PERL) -w $(srcdir)/$(PROTOCOL_GENERATOR) $(RPCGEN) -c $(SANDBOX_PROTOCOL_FILES:%=$(srcdir)/%) $@

libvirt-sandbox-protocol.h: $(srcdir)/$(PROTOCOL_GENERATOR) $(SANDBOX_PROTOCOL_FILES)
	$(AM_V_GEN)$(PERL) -w $(srcdir)/$(PROTOCOL_GENERATOR) $(RPCGEN) -h $(SANDBOX_PROTOCOL_FILES:%=$(srcdir)/%) $@


if WITH_INTROSPECTION

LibvirtSandbox-1.0.gir: libvirt-sandbox-1.0.la $(G_IR_SCANNER) Makefile.am
	$(AM_V_GEN)PKG_CONFIG_PATH=$(top_builddir) $(G_IR_SCANNER) \
                --quiet \
                --warn-all \
                --namespace LibvirtSandbox \
                --nsversion 1.0 \
                --include LibvirtGObject-1.0 \
                --identifier-prefix=GVirSandbox \
                --symbol-prefix=gvir_sandbox \
                --library=$(builddir)/libvirt-sandbox-1.0.la \
                --output $@ \
                -I$(top_srcdir) \
                -I$(top_builddir) \
                --verbose \
                --pkg=libvirt-gobject-1.0 \
                --c-include="libvirt-sandbox/libvirt-sandbox.h" \
                --pkg-export=libvirt-sandbox-1.0 \
                $(srcdir)/libvirt-sandbox.h \
                $(SANDBOX_SOURCE_FILES:%=$(srcdir)/%) \
                $(SANDBOX_HEADER_FILES:%=$(srcdir)/%) \
		$(SANDBOX_GENERATED_ENUM_FILES:%=%) \
		$(NULL)

girdir = $(datadir)/gir-1.0
gir_DATA = LibvirtSandbox-1.0.gir

typelibsdir = $(libdir)/girepository-1.0
typelibs_DATA = LibvirtSandbox-1.0.typelib

%.typelib: %.gir
	$(AM_V_GEN)$(G_IR_COMPILER) \
                --includedir=$(builddir) \
                --includedir=$(girdir) \
                -o $@ $<

CLEANFILES += $(gir_DATA) $(typelibs_DATA)

endif # WITH_INTROSPECTION
