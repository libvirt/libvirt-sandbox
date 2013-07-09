
SUBDIRS = libvirt-sandbox bin examples docs po

ACLOCAL_AMFLAGS = -I m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libvirt-sandbox-1.0.pc

EXTRA_DIST = \
	$(PACKAGE).spec \
	$(pkgconfig_DATA:%.pc=%.pc.in) \
	AUTHORS.in \
	$(NULL)

DISTCLEAN_FILES = $(PACKAGE).spec $(pkgconfig_DATA)

DISTCHECK_CONFIGURE_FLAGS = --enable-introspection=yes --enable-gtk-doc=yes

dist-hook: gen-ChangeLog gen-AUTHORS

# Generate the ChangeLog file (with all entries since the switch to git)
# and insert it into the directory we're about to use to create a tarball.
.PHONY: gen-ChangeLog gen-AUTHORS
gen-ChangeLog:
	if test -d .git || test -d ../.git; then					\
	  $(top_srcdir)/build-aux/gitlog-to-changelog		\
	     > $(distdir)/cl-t;	\
	  rm -f $(distdir)/ChangeLog;				\
	  mv $(distdir)/cl-t $(distdir)/ChangeLog;		\
	fi

gen-AUTHORS:
	$(AM_V_GEN)if test -d $(srcdir)/.git; then		\
	  out="`cd $(srcdir) && git log --pretty=format:'%aN <%aE>' | sort -u`" && \
	  perl -p -e "s/#authorslist#// and print '$$out'"	\
	    < $(srcdir)/AUTHORS.in > $(distdir)/AUTHORS-tmp &&	\
	  mv -f $(distdir)/AUTHORS-tmp $(distdir)/AUTHORS ;	\
	fi
