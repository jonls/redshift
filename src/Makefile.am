
SUBDIRS = redshift-gtk

# I18n
localedir = $(datadir)/locale
AM_CPPFLAGS = -DLOCALEDIR=\"$(localedir)\"

# redshift Program
bin_PROGRAMS = redshift

redshift_SOURCES = \
	colorramp.c colorramp.h \
	gamma-dummy.c gamma-dummy.h \
	hooks.c hooks.h \
	location-manual.c location-manual.h \
	options.c options.h \
	pipeutils.c pipeutils.h \
	redshift.c redshift.h \
	signals.c signals.h \
	solar.c solar.h \
	systemtime.c systemtime.h

EXTRA_redshift_SOURCES = \
	gamma-drm.c gamma-drm.h \
	gamma-randr.c gamma-randr.h \
	gamma-vidmode.c gamma-vidmode.h \
	gamma-quartz.c gamma-quartz.h \
	gamma-w32gdi.c gamma-w32gdi.h \
	location-geoclue2.c location-geoclue2.h \
	location-corelocation.m location-corelocation.h \
	windows/appicon.rc \
	windows/versioninfo.rc

# Note: rules for Elektra are based on https://github.com/kodebach/lcdproc/blob/3e2b9d3f5cade3bbe21fdb5a9ea1c9ffc1c994fa/server/Makefile.am
CLEANFILES = elektra/redshift-conf.c elektra/redshift-conf.h elektra/redshift-conf.mount.sh
nodist_redshift_SOURCES = elektra/redshift-conf.h elektra/redshift-conf.c
redshift.$(OBJEXT) options.$(OBJEXT): elektra/redshift-conf.h elektra/redshift-conf.c

if ENABLE_WINDOWS_RESOURCE
else
KDB ?= kdb
# Use pattern rule to ensure that kdb gen is only executed once, even if make -jN is used.
# see https://stackoverflow.com/a/10609434
%-conf.c %-conf.h %-conf.mount.sh: %.ni
	$(KDB) gen -F ni=$*.ni highlevel "/sw/jonls/redshift/#0/current" $*-conf initFn=loadConfiguration helpFn=printHelpMessage specloadFn=exitForSpecload embeddedSpec=full;
endif

scriptsdir = $(prefix)/libexec
scripts_DATA = elektra/redshift-conf.mount.sh

AM_CFLAGS =
redshift_LDADD = @LIBINTL@
EXTRA_DIST = windows/redshift.ico

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


if ENABLE_GEOCLUE2
redshift_SOURCES += location-geoclue2.c location-geoclue2.h
AM_CFLAGS += \
	$(GEOCLUE2_CFLAGS)
redshift_LDADD += \
	$(GEOCLUE2_LIBS) $(GEOCLUE2_CFLAGS)
endif

AM_CFLAGS += \
	$(ELEKTRA_CFLAGS)
redshift_LDADD += \
	$(ELEKTRA_LIBS) $(ELEKTRA_CFLAGS)


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


# Windows resources
if ENABLE_WINDOWS_RESOURCE
redshift_SOURCES += windows/appicon.rc windows/versioninfo.rc

endif

.rc.o:
	$(AM_V_GEN)$(WINDRES) -I$(top_builddir) -i $< -o $@
