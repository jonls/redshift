
# redshift
bin_PROGRAMS = redshift

redshift_SOURCES = \
	redshift.c \
	colortemp.c colortemp.h \
	solar.c solar.h

AM_CFLAGS = $(XCB_CFLAGS)
redshift_LDADD = -lm $(XCB_LIBS) $(XCB_CFLAGS)
