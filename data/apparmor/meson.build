if config_build.has('ENABLE_APPARMOR')
  usr_bin_redshift = configure_file(
    input : 'usr.bin.redshift.in',
    output : 'usr.bin.redshift',
    configuration : in_substs,
  )
  install_data(usr_bin_redshift,
    install_dir : join_paths(get_option('sysconfdir'), 'apparmor.d'),
  )
endif
