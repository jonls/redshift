if host_machine.system() != 'windows'
  if config_build.has('ENABLE_GUI')
    i18n.merge_file(
      input : 'redshift-gtk.appdata.xml.in',
      output : 'redshift-gtk.appdata.xml',
      po_dir : po_dir,
      install : true,
      install_dir : get_option('datadir') / 'appdata',
    )
  endif
endif
