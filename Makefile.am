
# redshift
bin_PROGRAMS = redshift

redshift_SOURCES = \
	redshift.c \
	colorramp.c colorramp.h \
	randr.c randr.h \
	solar.c solar.h

AM_CFLAGS = $(XCB_CFLAGS)
redshift_LDADD = -lm $(XCB_LIBS) $(XCB_CFLAGS)
