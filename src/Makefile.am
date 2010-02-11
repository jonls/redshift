
SUBDIRS = gtk-redshift

# I18n
localedir = $(datadir)/locale
INCLUDES = -DLOCALEDIR=\"$(localedir)\"

# redshift Program
bin_PROGRAMS = redshift

redshift_SOURCES = \
	redshift.c \
	colorramp.c colorramp.h \
	solar.c solar.h

EXTRA_redshift_SOURCES = \
	randr.c randr.h \
	vidmode.c vidmode.h

AM_CFLAGS =
redshift_LDADD = @LIBINTL@
EXTRA_DIST =

if ENABLE_RANDR
redshift_SOURCES += randr.c randr.h
AM_CFLAGS += $(XCB_CFLAGS) $(XCB_RANDR_CFLAGS)
redshift_LDADD += \
	$(XCB_LIBS) $(XCB_CFLAGS) \
	$(XCB_RANDR_LIBS) $(XCB_RANDR_CFLAGS)
endif

if ENABLE_VIDMODE
redshift_SOURCES += vidmode.c vidmode.h
AM_CFLAGS += $(X11_CFLAGS) $(XF86VM_CFLAGS)
redshift_LDADD += \
	$(X11_LIBS) $(X11_CFLAGS) \
	$(XF86VM_LIBS) $(XF86VM_CFLAGS)
endif
