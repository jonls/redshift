
if ENABLE_GTK
module=statusicon
gtk_redshift_PYTHON = \
	__init__.py \
	statusicon.py
nodist_gtk_redshift_PYTHON = \
	defs.py
gtk_redshiftdir = $(pythondir)/gtk_redshift

dist_bin_SCRIPTS = gtk-redshift
EXTRA_DIST = gtk-redshift.in \
    defs.py.in
endif

if ENABLE_APPINDICATOR
module=rsappindicator
gtk_redshift_PYTHON = \
	__init__.py \
	rsappindicator.py
nodist_gtk_redshift_PYTHON = \
	defs.py
gtk_redshiftdir = $(pythondir)/gtk_redshift

dist_bin_SCRIPTS = gtk-redshift
EXTRA_DIST = gtk-redshift.in \
    defs.py.in
endif

CLEANFILES = defs.py \
    gtk-redshift

# Local python definitions
gtk-redshift: gtk-redshift.in
	sed -e "s|\@module\@|$(module)|g" $< > $@

defs.py: defs.py.in
	sed -e "s|\@bindir\@|$(bindir)|g" \
		-e "s|\@localedir\@|$(localedir)|g" $< > $@
