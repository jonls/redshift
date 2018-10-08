redshift_libs = []
redshift_deps = [m_dep, intl_dep]
include_dirs = [configuration_inc]

libredshift_common_sources = files(
  'colorramp.c',
  'colorramp.h',
  'config-ini.c',
  'config-ini.h',
  'gamma-dummy.c',
  'gamma-dummy.h',
  'location-manual.c',
  'location-manual.h',
  'options.c',
  'options.h',
  'pipeutils.c',
  'pipeutils.h',
)

redshift_sources = files(
  'hooks.c',
  'hooks.h',
  'redshift.c',
  'redshift.h',
  'signals.c',
  'signals.h',
  'solar.c',
  'solar.h',
  'systemtime.c',
  'systemtime.h',
)

if config_h.has('ENABLE_DRM')
  libredshift_common_sources += files('gamma-drm.c', 'gamma-drm.h')
  redshift_deps += drm_dep
endif

if config_h.has('ENABLE_RANDR')
  libredshift_common_sources += files('gamma-randr.c', 'gamma-randr.h')
  redshift_deps += [xcb_randr_dep, xcb_dep]
endif

if config_h.has('ENABLE_VIDMODE')
  libredshift_common_sources += files('gamma-vidmode.c', 'gamma-vidmode.h')
  redshift_deps += [xxf86vm_dep, x11_dep]
endif

if config_h.has('ENABLE_QUARTZ')
  libredshift_common_sources += files('gamma-quartz.c', 'gamma-quartz.h')
  redshift_deps += appservices_dep
endif

if config_h.has('ENABLE_WINGDI')
  libredshift_common_sources += files('gamma-w32gdi.c', 'gamma-w32gdi.h')
  redshift_deps += gdi32_dep
endif

if config_h.has('ENABLE_GEOCLUE2')
  libredshift_common_sources += files('location-geoclue2.c', 'location-geoclue2.h')
  redshift_deps += gio_dep
endif

# Build CoreLocation module as a separate convenience
# library since it is using a separate compiler
# (Objective C).
if config_h.has('ENABLE_CORELOCATION')
  redshift_libs += static_library('libredshift_corelocation',
    ['location-corelocation.m', 'location-corelocation.h'],
    include_directories : include_dirs,
  )
  redshift_deps += [corelocation_dep, foundation_dep, cocoa_dep]
endif

if host_machine.system() == 'windows'
  windows = import('windows')
  # Windows resources
  win_resources = windows.compile_resources(
    ['windows/appicon.rc', 'windows/versioninfo.rc'],
    include_directories : include_dirs,
  )
  redshift_sources += win_resources
endif

libredshift_common = static_library('libredshift_common',
  libredshift_common_sources,
  dependencies : redshift_deps,
  include_directories : include_dirs,
)
redshift_libs += libredshift_common

executable('redshift',
  redshift_sources,
  include_directories : include_dirs,
  link_with : redshift_libs,
  install : true,
)

subdir('redshift-gtk')
