
SUBDIRS = redshift-gtk

# I18n
localedir = $(datadir)/locale
AM_CPPFLAGS = -DLOCALEDIR=\"$(localedir)\"

# redshift Program
bin_PROGRAMS = redshift

redshift_SOURCES = \
	redshift.c redshift.h \
	method.c method.h \
	colorramp.c colorramp.h \
	config-ini.c config-ini.h \
	location-manual.c location-manual.h \
	solar.c solar.h \
	systemtime.c systemtime.h \
	adjustments.h \
	gamma-common.c gamma-common.h \
	gamma-libgamma.c gamma-libgamma.h \
	opt-parser.c opt-parser.h

EXTRA_redshift_SOURCES = \
	location-geoclue.c location-geoclue.h

AM_CFLAGS =
redshift_LDADD = @LIBINTL@ -lgamma
EXTRA_DIST =


if ENABLE_GEOCLUE
redshift_SOURCES += location-geoclue.c location-geoclue.h
AM_CFLAGS += \
	$(GEOCLUE_CFLAGS) $(GEOCLUE_LIBS) \
	$(GLIB_CFLAGS) $(GLIB_LIBS)
redshift_LDADD += \
	$(GEOCLUE_LIBS) $(GEOCLUE_CFLAGS)
	$(GLIB_LIBS) $(GLIB_CFLAGS)
endif
