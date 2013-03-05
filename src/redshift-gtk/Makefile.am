
if ENABLE_GUI
redshift_gtk_PYTHON = \
	__init__.py \
	utils.py \
	statusicon.py
nodist_redshift_gtk_PYTHON = \
	defs.py
redshift_gtkdir = $(pythondir)/redshift_gtk

dist_bin_SCRIPTS = redshift-gtk
endif

EXTRA_DIST = defs.py.in
CLEANFILES = defs.py


# Local python definitions
defs.py: defs.py.in
	sed -e "s|\@bindir\@|$(bindir)|g" \
		-e "s|\@localedir\@|$(localedir)|g" $< > $@
