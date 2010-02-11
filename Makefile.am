
SUBDIRS = src po
ACLOCAL_AMFLAGS = -I m4

# Icons
icondir = @datadir@/icons/hicolor/scalable/apps
icon_DATA = data/icons/hicolor/scalable/apps/redshift.svg

EXTRA_DIST = $(icon_DATA)


# Update PO translations
.PHONY: update-po
update-po:
	cd po && $(MAKE) POTFILES redshift.pot update-po
