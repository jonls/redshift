
# redshift
bin_PROGRAMS = redshift

redshift_SOURCES = \
	src/redshift.c \
	src/colorramp.c src/colorramp.h \
	src/randr.c src/randr.h \
	src/solar.c src/solar.h

AM_CFLAGS = $(XCB_CFLAGS)
redshift_LDADD = -lm $(XCB_LIBS) $(XCB_CFLAGS)
