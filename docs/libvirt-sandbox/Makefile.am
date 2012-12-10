
DOC_MODULE=Libvirt-sandbox

DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.xml

DOC_SOURCE_DIR=$(top_srcdir)/libvirt-sandbox

SCANGOBJ_OPTIONS=

SCAN_OPTIONS=--rebuild-sections --rebuild-types

MKDB_OPTIONS=--sgml-mode --output-format=xml

MKTMPL_OPTIONS=

MKHTML_OPTIONS=

FIXXREF_OPTIONS=

HFILE_GLOB=$(top_srcdir)/libvirt-sandbox/*.h
CFILE_GLOB=$(top_srcdir)/libvirt-sandbox/*.c

IGNORE_HFILES=

HTML_IMAGES=

content_files=

expand_content_files=

GTKDOC_CFLAGS= \
	$(GIO_UNIX_CFLAGS) \
	$(LIBVIRT_GLIB_CFLAGS) \
	$(LIBVIRT_GOBJECT_CFLAGS)
GTKDOC_LIBS = \
	$(GIO_UNIX_LIBS) \
	$(LIBVIRT_GLIB_LIBS) \
	$(LIBVIRT_GOBJECT_LIBS) \
	$(top_builddir)/libvirt-sandbox/libvirt-sandbox-1.0.la

include $(top_srcdir)/gtk-doc.make
