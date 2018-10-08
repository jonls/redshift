# Icons
icons_dir = join_paths(get_option('datadir'), 'icons')
install_subdir(join_paths('icons', 'hicolor'), install_dir : icons_dir)
if config_build.has('ENABLE_UBUNTU')
  install_subdir(
    join_paths('icons', 'ubuntu-mono-dark'), install_dir : icons_dir)
  install_subdir(
    join_paths('icons', 'ubuntu-mono-light'), install_dir : icons_dir)
endif

subdir('apparmor')
subdir('appdata')
subdir('applications')
subdir('systemd')
