DOMAIN = $(PACKAGE_NAME)
COPYRIGHT_HOLDER = The Libvirt authors
MSGID_BUGS_ADDRESS = https://libvirt.org/bugs.html

LANGS := $(shell cat $(srcdir)/LINGUAS)

POTFILE := $(srcdir)/$(DOMAIN).pot
POFILES := $(LANGS:%=%.po)
GMOFILES := $(LANGS:%=%.gmo)

CLEANFILES = $(GMOFILES)

EXTRA_DIST = \
	LINGUAS \
	POTFILES \
	$(POTFILE) \
	$(POFILES)

if HAVE_GNU_GETTEXT_TOOLS

XGETTEXT_ARGS = \
	--default-domain=$(DOMAIN) \
	--from-code=utf-8 \
	--add-comments=TRANSLATORS: \
	--keyword=_ --keyword=N_ \
	--copyright-holder='$(COPYRIGHT_HOLDER)' \
	--package-name="$(PACKAGE_NAME)" \
	--package-version="$(PACKAGE_VERSION)" \
	--msgid-bugs-address="$(MSGID_BUGS_ADDRESS)" \
	--directory=$(top_srcdir) \
	--sort-output \
	$(NULL)

SED_PO_FIXUP_ARGS = \
	-e "s|text/plain; charset=CHARSET|text/plain; charset=UTF-8|g" \
	-e "s|SOME DESCRIPTIVE TITLE|Libvirt package strings|g" \
	-e "s|Copyright (C) YEAR|Copyright (C) $$(date +'%Y')|" \
	$(NULL)

libvirt-sandbox-pot:
	$(XGETTEXT) -o $(POTFILE)-t $(XGETTEXT_ARGS) \
	  --files-from=$(abs_srcdir)/POTFILES
	$(SED) $(SED_PO_FIXUP_ARGS) < $(POTFILE)-t > $(POTFILE)
	rm -f $(POTFILE)-t

%.gmo: %.po
	rm -f $@ $@-t
	$(MSGFMT) -c -o $@-t $<
	mv $@-t $@

endif HAVE_GNU_GETTEXT_TOOLS

if ENABLE_NLS

# Cannot use 'localedir' since this conflicts with autoconf.
langinstdir = $(datadir)/locale

install-data-hook: $(GMOFILES)
	mkdir -p $(DESTDIR)$(langinstdir)
	for lang in $(LANGS); do \
	  d=$(DESTDIR)$(langinstdir)/$$lang/LC_MESSAGES; \
	  mkdir -p $$d; \
	  install -m 0644 $$lang.gmo $$d/$(DOMAIN).mo; \
	done

uninstall-hook:
	for lang in $(LANGS); do \
	  d=$(DESTDIR)$(langinstdir)/$$lang/LC_MESSAGES; \
	  rm -f $$d/$(DOMAIN).mo; \
	done

endif ENABLE_NLS
