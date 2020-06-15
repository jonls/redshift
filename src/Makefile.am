
SUBDIRS = . redshift-gtk

if ENABLE_CONFIG_GUI
SUBDIRS += redshift-config-gtk
endif

# I18n
localedir = $(datadir)/locale
AM_CPPFLAGS = -DLOCALEDIR=\"$(localedir)\"

# redshift Program
bin_PROGRAMS = redshift

noinst_LTLIBRARIES = libredshift_common.la
libredshift_common_la_LIBADD =

libredshift_common_la_SOURCES = \
	colorramp.c colorramp.h \
	config-ini.c config-ini.h \
	gamma-dummy.c gamma-dummy.h \
	location-manual.c location-manual.h \
	options.c options.h \
	pipeutils.c pipeutils.h

EXTRA_libredshift_common_la_SOURCES = \
	gamma-drm.c gamma-drm.h \
	gamma-randr.c gamma-randr.h \
	gamma-vidmode.c gamma-vidmode.h \
	gamma-quartz.c gamma-quartz.h \
	gamma-w32gdi.c gamma-w32gdi.h \
	location-geoclue2.c location-geoclue2.h \
	location-corelocation.m location-corelocation.h

redshift_SOURCES = \
	hooks.c hooks.h \
	redshift.c redshift.h \
	signals.c signals.h \
	solar.c solar.h \
	systemtime.c systemtime.h

EXTRA_redshift_SOURCES = \
	windows/appicon.rc \
	windows/versioninfo.rc

AM_CFLAGS =
redshift_LDADD = @LIBINTL@ libredshift_common.la
EXTRA_DIST = windows/redshift.ico

if ENABLE_DRM
libredshift_common_la_SOURCES += gamma-drm.c gamma-drm.h
AM_CFLAGS += $(DRM_CFLAGS)
libredshift_common_la_LIBADD += \
	$(DRM_LIBS) $(DRM_CFLAGS)
endif

if ENABLE_RANDR
libredshift_common_la_SOURCES += gamma-randr.c gamma-randr.h
AM_CFLAGS += $(XCB_CFLAGS) $(XCB_RANDR_CFLAGS)
libredshift_common_la_LIBADD += \
	$(XCB_LIBS) $(XCB_CFLAGS) \
	$(XCB_RANDR_LIBS) $(XCB_RANDR_CFLAGS)
endif

if ENABLE_VIDMODE
libredshift_common_la_SOURCES += gamma-vidmode.c gamma-vidmode.h
AM_CFLAGS += $(X11_CFLAGS) $(XF86VM_CFLAGS)
libredshift_common_la_LIBADD += \
	$(X11_LIBS) $(X11_CFLAGS) \
	$(XF86VM_LIBS) $(XF86VM_CFLAGS)
endif

if ENABLE_QUARTZ
libredshift_common_la_SOURCES += gamma-quartz.c gamma-quartz.h
AM_CFLAGS += $(QUARTZ_CFLAGS)
libredshift_common_la_LIBADD += \
	$(QUARTZ_LIBS) $(QUARTZ_CFLAGS)
endif

if ENABLE_WINGDI
libredshift_common_la_SOURCES += gamma-w32gdi.c gamma-w32gdi.h
libredshift_common_la_LIBADD += -lgdi32
endif


if ENABLE_GEOCLUE2
libredshift_common_la_SOURCES += location-geoclue2.c location-geoclue2.h
AM_CFLAGS += \
	$(GEOCLUE2_CFLAGS)
libredshift_common_la_LIBADD += \
	$(GEOCLUE2_LIBS) $(GEOCLUE2_CFLAGS)
endif

# Build CoreLocation module as a separate convenience
# library since it is using a separate compiler
# (Objective C).

if ENABLE_CORELOCATION
noinst_LTLIBRARIES += liblocation-corelocation.la
liblocation_corelocation_la_SOURCES = \
	location-corelocation.m location-corelocation.h
liblocation_corelocation_la_OBJCFLAGS = \
	$(CORELOCATION_CFLAGS)
liblocation_corelocation_la_LIBADD = \
	$(CORELOCATION_CFLAGS) $(CORELOCATION_LIBS)
libredshift_common_la_LIBADD += liblocation-corelocation.la
endif


# Windows resources
if ENABLE_WINDOWS_RESOURCE
redshift_SOURCES += windows/appicon.rc windows/versioninfo.rc
endif

.rc.o:
	$(AM_V_GEN)$(WINDRES) -I$(top_builddir) -i $< -o $@
