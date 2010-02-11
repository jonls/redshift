
SUBDIRS = po
ACLOCAL_AMFLAGS = -I m4

# I18n
localedir = $(datadir)/locale
INCLUDES = -DLOCALEDIR=\"$(localedir)\"

# redshift Program
bin_PROGRAMS = redshift

redshift_SOURCES = \
	src/redshift.c \
	src/colorramp.c src/colorramp.h \
	src/solar.c src/solar.h

EXTRA_redshift_SOURCES = \
	src/randr.c src/randr.h \
	src/vidmode.c src/vidmode.h

AM_CFLAGS =
redshift_LDADD = @LIBINTL@
EXTRA_DIST =

if ENABLE_RANDR
redshift_SOURCES += src/randr.c src/randr.h
AM_CFLAGS += $(XCB_CFLAGS) $(XCB_RANDR_CFLAGS)
redshift_LDADD += \
	$(XCB_LIBS) $(XCB_CFLAGS) \
	$(XCB_RANDR_LIBS) $(XCB_RANDR_CFLAGS)
endif

if ENABLE_VIDMODE
redshift_SOURCES += src/vidmode.c src/vidmode.h
AM_CFLAGS += $(X11_CFLAGS) $(XF86VM_CFLAGS)
redshift_LDADD += \
	$(X11_LIBS) $(X11_CFLAGS) \
	$(XF86VM_LIBS) $(XF86VM_CFLAGS)
endif

# gtk-redshift Script
if ENABLE_GTK
gtk_redshift_PYTHON = \
	src/gtk-redshift/__init__.py \
	src/gtk-redshift/statusicon.py \
	src/gtk-redshift/defs.py
gtk_redshiftdir = $(pythondir)/gtk_redshift

dist_bin_SCRIPTS = src/gtk-redshift/gtk-redshift
EXTRA_DIST += src/gtk-redshift/defs.py.in
endif

# Icons
icondir = @datadir@/icons/hicolor/scalable/apps
icon_DATA = data/icons/hicolor/scalable/apps/redshift.svg

EXTRA_DIST += $(icon_DATA)


CLEANFILES = src/gtk-redshift/defs.py

# Local python definitions
src/gtk-redshift/defs.py: src/gtk-redshift/defs.py.in
	sed -e "s|\@localedir\@|$(localedir)|g" $< > $@

# Update PO translations
.PHONY: update-po
update-po:
	cd po && $(MAKE) POTFILES redshift.pot update-po
