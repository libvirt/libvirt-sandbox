
EXTRA_DIST = libvirt-sandbox.sym
CLEANFILES =

lib_LTLIBRARIES = libvirt-sandbox-1.0.la

SANDBOX_HEADER_FILES = \
			libvirt-sandbox.h \
			libvirt-sandbox-config.h \
			libvirt-sandbox-config-graphical.h \
			libvirt-sandbox-config-mount.h \
			libvirt-sandbox-cleaner.h \
			libvirt-sandbox-builder.h \
			libvirt-sandbox-builder-machine.h \
			libvirt-sandbox-builder-container.h \
			libvirt-sandbox-context.h \
			$(NULL)
SANDBOX_SOURCE_FILES = \
			libvirt-sandbox-config.c \
			libvirt-sandbox-config-graphical.c \
			libvirt-sandbox-config-mount.c \
			libvirt-sandbox-cleaner.c \
			libvirt-sandbox-builder.c \
			libvirt-sandbox-builder-machine.c \
			libvirt-sandbox-builder-container.c \
			libvirt-sandbox-context.c \
			$(NULL)

libvirt_sandbox_1_0_ladir = $(includedir)/libvirt-sandbox-1.0/libvirt-sandbox
libvirt_sandbox_1_0_la_HEADERS = \
			$(SANDBOX_HEADER_FILES)
libvirt_sandbox_1_0_la_SOURCES = \
			$(SANDBOX_SOURCE_FILES)
libvirt_sandbox_1_0_la_CFLAGS = \
			-DDATADIR="\"$(datadir)\"" \
			-DLIBVIRT_SANDBOX_BUILD \
			-DLIBEXECDIR="\"$(libexecdir)\"" \
			$(COVERAGE_CFLAGS) \
			-I$(top_srcdir) \
			$(LIBVIRT_GOBJECT_CFLAGS) \
			$(WARN_CFLAGS)
libvirt_sandbox_1_0_la_LIBADD = \
			$(LIBVIRT_GOBJECT_LIBS) \
			$(CYGWIN_EXTRA_LIBADD)
libvirt_sandbox_1_0_la_DEPENDENCIES = \
                        libvirt-sandbox.sym
libvirt_sandbox_1_0_la_LDFLAGS = \
			$(WARN_CFLAGS) \
			$(COVERAGE_CFLAGS:-f%=-Wc,f%) \
			$(CYGWIN_EXTRA_LDFLAGS) \
			$(MINGW_EXTRA_LDFLAGS) \
			-Wl,--version-script=$(srcdir)/libvirt-sandbox.sym \
			-version-info $(LIBVIRT_SANDBOX_VERSION_INFO)

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
                $(SANDBOX_HEADER_FILES:%=$(srcdir)/%)

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
