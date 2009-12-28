
# redshift
bin_PROGRAMS = redshift

redshift_SOURCES = \
	src/redshift.c \
	src/colorramp.c src/colorramp.h \
	src/solar.c src/solar.h

EXTRA_redshift_SOURCES = \
	src/randr.c src/randr.h \
	src/vidmode.c src/vidmode.h

AM_CFLAGS =
redshift_LDADD = -lm -lrt

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
