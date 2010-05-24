
SUBDIRS = gtk-redshift

# I18n
localedir = $(datadir)/locale
INCLUDES = -DLOCALEDIR=\"$(localedir)\"

# redshift Program
bin_PROGRAMS = redshift

redshift_SOURCES = \
	redshift.c \
	colorramp.c colorramp.h \
	solar.c solar.h \
	systemtime.c systemtime.h \
	location-manual.c location-manual.h

EXTRA_redshift_SOURCES = \
	gamma-randr.c gamma-randr.h \
	gamma-vidmode.c gamma-vidmode.h \
	gamma-w32gdi.c gamma-w32gdi.h \
	location-gnome-clock.c location-gnome-clock.h

AM_CFLAGS =
redshift_LDADD = @LIBINTL@
EXTRA_DIST =

if ENABLE_RANDR
redshift_SOURCES += gamma-randr.c gamma-randr.h
AM_CFLAGS += $(XCB_CFLAGS) $(XCB_RANDR_CFLAGS)
redshift_LDADD += \
	$(XCB_LIBS) $(XCB_CFLAGS) \
	$(XCB_RANDR_LIBS) $(XCB_RANDR_CFLAGS)
endif

if ENABLE_VIDMODE
redshift_SOURCES += gamma-vidmode.c gamma-vidmode.h
AM_CFLAGS += $(X11_CFLAGS) $(XF86VM_CFLAGS)
redshift_LDADD += \
	$(X11_LIBS) $(X11_CFLAGS) \
	$(XF86VM_LIBS) $(XF86VM_CFLAGS)
endif

if ENABLE_WINGDI
redshift_SOURCES += gamma-w32gdi.c gamma-w32gdi.h
redshift_LDADD += -lgdi32
endif

if ENABLE_GNOME_CLOCK
redshift_SOURCES += location-gnome-clock.c location-gnome-clock.h
AM_CFLAGS += $(GLIB_CFLAGS) $(GCONF_CFLAGS)
redshift_LDADD += \
	$(GLIB_LIBS) $(GLIB_CFLAGS) \
	$(GCONF_LIBS) $(GCONF_CFLAGS)
endif
