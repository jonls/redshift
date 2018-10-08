if config_build.has('ENABLE_SYSTEMD')
  redshift_service = configure_file(
    input : 'redshift.service.in',
    output : 'redshift.service',
    configuration : in_substs,
  )
  install_data(redshift_service, install_dir : systemd_userunitdir)
  if config_build.has('ENABLE_GUI')
    redshift_gtk_service = configure_file(
      input : 'redshift-gtk.service.in',
      output : 'redshift-gtk.service',
      configuration : in_substs,
    )
    install_data(redshift_gtk_service, install_dir : systemd_userunitdir)
  endif
endif
