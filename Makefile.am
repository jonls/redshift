
# redshift
bin_PROGRAMS = redshift

redshift_SOURCES = \
	src/redshift.c \
	src/colorramp.c src/colorramp.h \
	src/randr.c src/randr.h \
	src/vidmode.c src/vidmode.h \
	src/solar.c src/solar.h

AM_CFLAGS = $(X11_CFLAGS) $(XF86VM_CFLAGS) $(XCB_CFLAGS)
redshift_LDADD = -lm \
	$(X11_LIBS) $(X11_CFLAGS) \
	$(XF86VM_LIBS) $(XF86VM_CFLAGS) \
	$(XCB_LIBS) $(XCB_CFLAGS)
