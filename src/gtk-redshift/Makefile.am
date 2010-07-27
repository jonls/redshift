
if ENABLE_GUI
gtk_redshift_PYTHON = \
	__init__.py \
	utils.py \
	statusicon.py
nodist_gtk_redshift_PYTHON = \
	defs.py
gtk_redshiftdir = $(pythondir)/gtk_redshift

dist_bin_SCRIPTS = gtk-redshift
endif

EXTRA_DIST = defs.py.in
CLEANFILES = defs.py


# Local python definitions
defs.py: defs.py.in
	sed -e "s|\@bindir\@|$(bindir)|g" \
		-e "s|\@localedir\@|$(localedir)|g" $< > $@
