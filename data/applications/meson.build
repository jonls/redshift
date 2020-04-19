if host_machine.system() != 'windows'
  desktop_dir = get_option('datadir') / 'applications'

  i18n.merge_file(
    input : 'redshift.desktop.in',
    output : 'redshift.desktop',
    type : 'desktop',
    po_dir : po_dir,
    install : true,
    install_dir : desktop_dir,
  )

  if config_build.has('ENABLE_GUI')
    i18n.merge_file(
      input : 'redshift-gtk.desktop.in',
      output : 'redshift-gtk.desktop',
      type : 'desktop',
      po_dir : po_dir,
      install : true,
      install_dir : desktop_dir,
    )
  endif
endif
