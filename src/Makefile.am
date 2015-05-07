
SUBDIRS = redshift-gtk

# I18n
localedir = $(datadir)/locale
AM_CPPFLAGS = -DLOCALEDIR=\"$(localedir)\"

# redshift Program
bin_PROGRAMS = redshift

redshift_SOURCES = \
	settings.c settings.h \
	redshift.c redshift.h \
	colorramp.c colorramp.h \
	config-ini.c config-ini.h \
	location-manual.c location-manual.h \
	solar.c solar.h \
	systemtime.c systemtime.h \
	hooks.c hooks.h \
	gamma-dummy.c gamma-dummy.h

EXTRA_redshift_SOURCES = \
	gamma-drm.c gamma-drm.h \
	gamma-randr.c gamma-randr.h \
	gamma-vidmode.c gamma-vidmode.h \
	gamma-quartz.c gamma-quartz.h \
	gamma-w32gdi.c gamma-w32gdi.h \
	location-geoclue.c location-geoclue.h

AM_CFLAGS =
redshift_LDADD = @LIBINTL@
EXTRA_DIST =

if ENABLE_DRM
redshift_SOURCES += gamma-drm.c gamma-drm.h
AM_CFLAGS += $(DRM_CFLAGS)
redshift_LDADD += \
	$(DRM_LIBS) $(DRM_CFLAGS)
endif

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

if ENABLE_QUARTZ
redshift_SOURCES += gamma-quartz.c gamma-quartz.h
AM_CFLAGS += $(QUARTZ_CFLAGS)
redshift_LDADD += \
	$(QUARTZ_LIBS) $(QUARTZ_CFLAGS)
endif

if ENABLE_WINGDI
redshift_SOURCES += gamma-w32gdi.c gamma-w32gdi.h
redshift_LDADD += -lgdi32
endif


if ENABLE_GEOCLUE
redshift_SOURCES += location-geoclue.c location-geoclue.h
AM_CFLAGS += \
	$(GEOCLUE_CFLAGS) $(GEOCLUE_LIBS) \
	$(GLIB_CFLAGS) $(GLIB_LIBS)
redshift_LDADD += \
	$(GEOCLUE_LIBS) $(GEOCLUE_CFLAGS)
	$(GLIB_LIBS) $(GLIB_CFLAGS)
endif

if ENABLE_GEOCLUE2
redshift_SOURCES += location-geoclue2.c location-geoclue2.h
AM_CFLAGS += \
	$(GEOCLUE2_CFLAGS)
redshift_LDADD += \
	$(GEOCLUE2_LIBS) $(GEOCLUE2_CFLAGS)
endif

# Build CoreLocation module as a separate convenience
# library since it is using a separate compiler
# (Objective C).

if ENABLE_CORELOCATION
noinst_LTLIBRARIES = liblocation-corelocation.la
liblocation_corelocation_la_SOURCES = \
	location-corelocation.m location-corelocation.h
liblocation_corelocation_la_OBJCFLAGS = \
	$(CORELOCATION_CFLAGS)
liblocation_corelocation_la_LIBADD = \
	$(CORELOCATION_CFLAGS) $(CORELOCATION_LIBS)
redshift_LDADD += liblocation-corelocation.la
endif
