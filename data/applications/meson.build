if host_machine.system() != 'windows'
  desktop_dir = join_paths(get_option('datadir'), 'applications')

  i18n.merge_file(
    input : 'meson_redshift.desktop.in',
    output : 'redshift.desktop',
    type : 'desktop',
    po_dir : po_dir,
    install : true,
    install_dir : desktop_dir,
  )

  if config_build.has('ENABLE_GUI')
    i18n.merge_file(
      input : 'meson_redshift-gtk.desktop.in',
      output : 'redshift-gtk.desktop',
      type : 'desktop',
      po_dir : po_dir,
      install : true,
      install_dir : desktop_dir,
    )
  endif
endif
