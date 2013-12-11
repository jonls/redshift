
SUBDIRS = src po
ACLOCAL_AMFLAGS = -I m4

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

# man page
dist_man1_MANS = redshift.1

EXTRA_DIST = \
	$(EXTRA_ROOTDOC_FILES) \
	$(_HICOLOR_FILES) \
	$(_UBUNTU_MONO_DARK_FILES) \
	$(_UBUNTU_MONO_LIGHT_FILES) \
	$(_DESKTOP_FILES)

# Update PO translations
.PHONY: update-po
update-po:
	cd po && $(MAKE) POTFILES redshift.pot update-po
