SUFFIXES = .gresource.xml

localedir = $(datadir)/locale
AM_CPPFLAGS = -DLOCALEDIR=\"$(localedir)\"

bin_PROGRAMS = redshift-config-gtk

redshift_config_gtk_SOURCES = \
	redshift-config-gtk.c

nodist_redshift_config_gtk_SOURCES = \
	redshift-config-gtk-resource.c

# GLib resources
redshift-config-gtk-resource.c: \
	redshift-config-gtk-resource.gresource.xml \
	gtk/menus.ui \
	icons/48x48/apps/redshift.png

redshift_config_gtk_CFLAGS = $(GTK3_CFLAGS) -I$(top_srcdir)/src
redshift_config_gtk_LDADD = $(GTK3_LIBS) ../libredshift_common.la

EXTRA_DIST = \
	redshift-config-gtk-resource.gresource.xml \
	gtk/menus.ui \
	icons/48x48/apps/redshift.png

.gresource.xml.c:
	$(AM_V_GEN)glib-compile-resources \
		--sourcedir=`dirname $<` $< --target=$@ --generate-source

CLEANFILES = \
	$(nodist_redshift_config_gtk_SOURCES)
