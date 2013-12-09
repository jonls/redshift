
SUBDIRS = src po
ACLOCAL_AMFLAGS = -I m4

# Install systemd user unit files locally for distcheck
DISTCHECK_CONFIGURE_FLAGS = \
	--with-systemduserunitdir=$$dc_install_base/$(systemduserunitdir)

UPDATE_ICON_CACHE = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor || :

EXTRA_ROOTDOC_FILES = \
	HACKING \
	DESIGN \
	README-colorramp

_HICOLOR_FILES = \
	data/icons/hicolor/scalable/apps/redshift.svg \
	data/icons/hicolor/scalable/apps/redshift-status-on.svg \
	data/icons/hicolor/scalable/apps/redshift-status-off.svg

_UBUNTU_MONO_DARK_FILES = \
	data/icons/ubuntu-mono-dark/scalable/apps/redshift-status-on.svg \
	data/icons/ubuntu-mono-dark/scalable/apps/redshift-status-off.svg

_UBUNTU_MONO_LIGHT_FILES = \
	data/icons/ubuntu-mono-light/scalable/apps/redshift-status-on.svg \
	data/icons/ubuntu-mono-light/scalable/apps/redshift-status-off.svg

_DESKTOP_FILES = \
	data/applications/redshift-gtk.desktop

SYSTEMD_USER_UNIT_IN_FILES = \
	data/systemd/redshift.service.in \
	data/systemd/redshift-gtk.service.in

DBUS_SERVICE_IN_FILES = \
	data/dbus-1/services/dk.jonls.redshift.Redshift.service.in


# Icons
if ENABLE_GUI
hicolor_icondir = @datadir@/icons/hicolor/scalable/apps
hicolor_icon_DATA = $(_HICOLOR_FILES)

if ENABLE_UBUNTU
ubuntu_mono_dark_icondir = @datadir@/icons/ubuntu-mono-dark/scalable/apps
ubuntu_mono_dark_icon_DATA = $(_UBUNTU_MONO_DARK_FILES)

ubuntu_mono_light_icondir = @datadir@/icons/ubuntu-mono-light/scalable/apps
ubuntu_mono_light_icon_DATA = $(_UBUNTU_MONO_LIGHT_FILES)
endif
endif


# Desktop file
if ENABLE_GUI
desktopdir = @datadir@/applications
desktop_DATA = $(_DESKTOP_FILES)
endif

if ENABLE_GUI
install-data-hook:
	$(UPDATE_ICON_CACHE);

uninstall-hook:
	$(UPDATE_ICON_CACHE);
endif


# man page
dist_man1_MANS = redshift.1


# Systemd service files
if ENABLE_SYSTEMD
systemduserunit_DATA = $(SYSTEMD_USER_UNIT_IN_FILES:.service.in=.service)
endif

$(systemduserunit_DATA): $(SYSTEMD_USER_UNIT_IN_FILES) Makefile
	$(AM_V_GEN)$(MKDIR_P) $(@D) && \
		sed -e "s|\@bindir\@|$(bindir)|g" $< > $@


# D-Bus service file
if ENABLE_DBUS
dbus_servicedir = @datadir@/dbus-1/services
dbus_service_DATA = $(DBUS_SERVICE_IN_FILES:.service.in=.service)
endif

$(dbus_service_DATA): $(DBUS_SERVICE_IN_FILES) Makefile
	$(AM_V_GEN)$(MKDIR_P) $(@D) && \
		sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@


EXTRA_DIST = \
	$(EXTRA_ROOTDOC_FILES) \
	$(_HICOLOR_FILES) \
	$(_UBUNTU_MONO_DARK_FILES) \
	$(_UBUNTU_MONO_LIGHT_FILES) \
	$(_DESKTOP_FILES) \
	$(SYSTEMD_USER_UNIT_IN_FILES) \
	$(DBUS_SERVICE_IN_FILES)

CLEANFILES = $(systemduserunit_DATA) \
	$(dbus_service_DATA)


# Update PO translations
.PHONY: update-po
update-po:
	cd po && $(MAKE) POTFILES redshift.pot update-po
