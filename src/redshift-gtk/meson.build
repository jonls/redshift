if config_build.has('ENABLE_GUI')
  py_installdir = join_paths(py_installation.get_install_dir(), 'redshift_gtk')
  in_substs.set('pythondir', py_installdir)

  defs_py = configure_file(
    input : 'defs.py.in',
    output : 'defs.py',
    configuration : in_substs,
  )
  redshift_gtk = configure_file(
    input : 'redshift-gtk.in',
    output : 'redshift-gtk',
    configuration : in_substs,
  )
  redshift_gtk_sources = files(
    '__init__.py',
    'controller.py',
    'statusicon.py',
    'utils.py',
  )

  py_installation.install_sources(redshift_gtk_sources, subdir : 'redshift_gtk')
  install_data(defs_py, install_dir : py_installdir)
  install_data(redshift_gtk,
    install_dir : full_bindir,
    install_mode : 'rwxr-xr-x',
  )
  meson.add_install_script('meson_compile_py.py', py_installdir)
endif
