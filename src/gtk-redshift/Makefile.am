
if ENABLE_STATUSICON
gui_module=statusicon
gtk_redshift_PYTHON = \
	__init__.py \
	statusicon.py
nodist_gtk_redshift_PYTHON = \
	defs.py
gtk_redshiftdir = $(pythondir)/gtk_redshift

bin_SCRIPTS = gtk-redshift
endif

if ENABLE_APPINDICATOR
gui_module=rsappindicator
gtk_redshift_PYTHON = \
	__init__.py \
	rsappindicator.py
nodist_gtk_redshift_PYTHON = \
	defs.py
gtk_redshiftdir = $(pythondir)/gtk_redshift

bin_SCRIPTS = gtk-redshift
endif

EXTRA_DIST = gtk-redshift.in \
	defs.py.in

CLEANFILES = defs.py \
	gtk-redshift


# Main GUI script
gtk-redshift: gtk-redshift.in
	sed -e "s|\@gui_module\@|$(gui_module)|g" $< > $@

# Local python definitions
defs.py: defs.py.in
	sed -e "s|\@bindir\@|$(bindir)|g" \
		-e "s|\@localedir\@|$(localedir)|g" $< > $@
