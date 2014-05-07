
SUBDIRS = redshift-gtk

# I18n
localedir = $(datadir)/locale
AM_CPPFLAGS = -DLOCALEDIR=\"$(localedir)\"

# redshift Program
bin_PROGRAMS = redshift

# redshift-dbus Program
if ENABLE_DBUS
libexec_PROGRAMS = redshift-dbus
endif

redshift_SOURCES = \
	redshift.c redshift.h \
	colorramp.c colorramp.h \
	config-ini.c config-ini.h \
	location-manual.c location-manual.h \
	solar.c solar.h \
	systemtime.c systemtime.h \
	gamma-dummy.c gamma-dummy.h

redshift_dbus_SOURCES = \
	redshift-dbus.c \
	colorramp.c colorramp.h \
	solar.c solar.h \
	gamma-dummy.c gamma-dummy.h

EXTRA_redshift_SOURCES = \
	gamma-drm.c gamma-drm.h \
	gamma-randr.c gamma-randr.h \
	gamma-vidmode.c gamma-vidmode.h \
	gamma-w32gdi.c gamma-w32gdi.h \
	location-geoclue.c location-geoclue.h


EXTRA_DIST =


redshift_CFLAGS =
redshift_LDADD = @LIBINTL@
redshift_dbus_CFLAGS = \
	$(GLIB_CFLAGS) $(GDBUS_CFLAGS) \
	$(GEOCLUE_CFLAGS)
redshift_dbus_LDADD = \
	$(GLIB_LIBS) $(GDBUS_LIBS) \
	$(GEOCLUE_LIBS)


if ENABLE_DRM
redshift_SOURCES += gamma-drm.c gamma-drm.h
redshift_CFLAGS += $(DRM_CFLAGS)
redshift_LDADD += \
	$(DRM_LIBS) $(DRM_CFLAGS)
endif

if ENABLE_RANDR
redshift_SOURCES += gamma-randr.c gamma-randr.h
redshift_CFLAGS += $(XCB_CFLAGS) $(XCB_RANDR_CFLAGS)
redshift_LDADD += \
	$(XCB_LIBS) $(XCB_CFLAGS) \
	$(XCB_RANDR_LIBS) $(XCB_RANDR_CFLAGS)

redshift_dbus_SOURCES += gamma-randr.c gamma-randr.h
redshift_dbus_CFLAGS += $(XCB_CFLAGS) $(XCB_RANDR_CFLAGS)
redshift_dbus_LDADD += \
	$(XCB_LIBS) $(XCB_CFLAGS) \
	$(XCB_RANDR_LIBS) $(XCB_RANDR_CFLAGS)
endif

if ENABLE_VIDMODE
redshift_SOURCES += gamma-vidmode.c gamma-vidmode.h
redshift_CFLAGS += $(X11_CFLAGS) $(XF86VM_CFLAGS)
redshift_LDADD += \
	$(X11_LIBS) $(X11_CFLAGS) \
	$(XF86VM_LIBS) $(XF86VM_CFLAGS)

redshift_dbus_SOURCES += gamma-vidmode.c gamma-vidmode.h
redshift_dbus_CFLAGS += $(X11_CFLAGS) $(XF86VM_CFLAGS)
redshift_dbus_LDADD += \
	$(X11_LIBS) $(X11_CFLAGS) \
	$(XF86VM_LIBS) $(XF86VM_CFLAGS)
endif

if ENABLE_WINGDI
redshift_SOURCES += gamma-w32gdi.c gamma-w32gdi.h
redshift_LDADD += -lgdi32
endif


if ENABLE_GEOCLUE
redshift_SOURCES += location-geoclue.c location-geoclue.h
redshift_CFLAGS += $(GEOCLUE_CFLAGS)
redshift_LDADD += \
	$(GEOCLUE_LIBS) $(GEOCLUE_CFLAGS)
endif
