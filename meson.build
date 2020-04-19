project(
  'redshift',
  'c',
  default_options : ['buildtype=debugoptimized', 'c_std=gnu99'],
  license : 'GPL3+',
  meson_version : '>= 0.49',
  version : '1.12',
)
proj_name = meson.project_name()
proj_version = meson.project_version()
proj_bugreport = 'https://github.com/jonls/redshift/issues'

# TODO: Currently unity build fails due to redefinition error.
#       (hooks.c and redshift.c have "static" variable "period_names")
if meson.is_unity()
  error('Sorry, unity build is currently broken.')
endif

cc = meson.get_compiler('c')

# Directories
full_bindir = get_option('prefix') / get_option('bindir')
full_localedir = get_option('prefix') / get_option('localedir')
po_dir = meson.source_root() / 'po'
in_substs = configuration_data()
in_substs.set('bindir', full_bindir)
in_substs.set('localedir', full_localedir)

# config.h
configuration_inc = include_directories('.')
add_project_arguments('-DHAVE_CONFIG_H', language : ['c', 'objc'])
config_h = configuration_data()
config_h.set_quoted('GETTEXT_PACKAGE', proj_name)
config_h.set_quoted('PACKAGE', proj_name)
config_h.set_quoted('PACKAGE_BUGREPORT', proj_bugreport)
config_h.set_quoted('PACKAGE_NAME', proj_name)
config_h.set_quoted('PACKAGE_STRING', '@0@ @1@'.format(proj_name, proj_version))
config_h.set_quoted('PACKAGE_TARNAME', proj_name)
config_h.set_quoted('PACKAGE_VERSION', proj_version)
config_h.set_quoted('LOCALEDIR', full_localedir)

# Check for library functions
intl_dep = cc.find_library('intl', required : false)
m_dep = cc.find_library('m', required : false)
if cc.sizeof('uint16_t', prefix : '#include <stdint.h>') == -1
  error('uint16_t is not defined.')
endif
foreach f : ['setlocale', 'strchr', 'floor', 'pow']
  if not cc.has_function(f, dependencies : m_dep)
    error('Function "@0@" is not available.'.format(f))
  endif
endforeach

# NLS
opt_nls = get_option('nls')
if not opt_nls.disabled()
  have_gettext_func = cc.has_function('gettext', dependencies : intl_dep)
  if not have_gettext_func and opt_nls.enabled()
    error('Function gettext() is not available.')
  endif
  have_libintl_h = cc.has_header('libintl.h')
  if not have_libintl_h and opt_nls.enabled()
    error('libintl.h is not installed.')
  endif
  if have_gettext_func and have_libintl_h
    config_h.set('ENABLE_NLS', 1)
  endif
endif

# Check for header files
system_headers = [
  'locale.h',
  'signal.h',
  'stdint.h',
  'stdlib.h',
  'string.h',
  'unistd.h',
]
foreach h : system_headers
  if cc.has_header(h)
    config_h.set('HAVE_@0@'.format(h.to_upper().underscorify()), 1)
  endif
endforeach

# Build configuration
config_build = configuration_data()

null_dep = disabler()

# Check DRM method
drm_dep = dependency('libdrm', required : get_option('drm'))
if drm_dep.found()
  config_h.set('ENABLE_DRM', 1)
endif

# Check RANDR method
xcb_dep = null_dep
xcb_randr_dep = dependency('xcb-randr', required : get_option('randr'))
if xcb_randr_dep.found()
  xcb_dep = dependency('xcb')
endif
if xcb_dep.found()
  config_h.set('ENABLE_RANDR', 1)
endif

# Check VidMode method
x11_dep = null_dep
xxf86vm_dep = dependency('xxf86vm', required : get_option('vidmode'))
if xxf86vm_dep.found()
  x11_dep = dependency('x11')
endif
if x11_dep.found()
  config_h.set('ENABLE_VIDMODE', 1)
endif

# Check Quartz (macOS) method
appservices_dep = dependency('appleframeworks',
  modules : 'applicationservices', required : get_option('quartz'))
if appservices_dep.found() and cc.has_header(
    'ApplicationServices/ApplicationServices.h')
  config_h.set('ENABLE_QUARTZ', 1)
endif

# Check Windows GDI method
gdi32_dep = cc.find_library('gdi32', required : get_option('wingdi'))
if gdi32_dep.found() and cc.has_header('windows.h')
  config_h.set('ENABLE_WINGDI', 1)
endif

# Check Geoclue2 location provider
gio_dep = dependency(
  'gio-2.0', version : '>= 2.26', required : get_option('geoclue2'))
if gio_dep.found()
  config_h.set('ENABLE_GEOCLUE2', 1)
endif

# Check CoreLocation (macOS) provider
foundation_dep = null_dep
cocoa_dep = null_dep
corelocation_dep = dependency('appleframeworks',
  modules : 'corelocation', required : get_option('corelocation'))
if corelocation_dep.found()
  foundation_dep = dependency('appleframeworks', modules : 'foundation')
  cocoa_dep = dependency('appleframeworks', modules : 'cocoa')
endif
if foundation_dep.found() and cocoa_dep.found()
  add_languages('objc')
  objc = meson.get_compiler('objc')
  if objc.has_header('CoreLocation/CoreLocation.h')
    config_h.set('ENABLE_CORELOCATION', 1)
  endif
endif

# Check for GUI status icon
pymod = import('python')
py_installation = pymod.find_installation('python3', required : get_option('gui'))
if py_installation.found()
  config_build.set('ENABLE_GUI', 1)
endif

# Check for Ubuntu icons
if get_option('ubuntu').enabled()
  config_build.set('ENABLE_UBUNTU', 1)
endif

# Check for systemd
systemd_userunitdir = ''
opt_systemd_userunitdir = get_option('systemduserunitdir')
if opt_systemd_userunitdir != 'disabled'
  if opt_systemd_userunitdir == 'auto'
    if host_machine.system() == 'linux'
      pkgconfig = find_program('pkg-config', required : false)
      if pkgconfig.found()
        cmd = run_command(pkgconfig, '--variable=systemduserunitdir', 'systemd')
        if cmd.returncode() == 0
          systemd_userunitdir = cmd.stdout().strip()
          config_build.set('ENABLE_SYSTEMD', 1)
        endif
      endif
    endif
  else
    systemd_userunitdir = opt_systemd_userunitdir
    config_build.set('ENABLE_SYSTEMD', 1)
  endif
endif

# Check for AppArmor
if get_option('apparmor').enabled()
  config_build.set('ENABLE_APPARMOR', 1)
endif

# i18n (used in subdirs)
i18n = import('i18n')

# Man
install_man('redshift.1')

# Subdirectories
subdir('src')
subdir('po')
subdir('data')

# Generate config.h
configure_file(output : 'config.h', configuration : config_h)

message('''@0@ @1@
    prefix:		@2@

    Adjustment methods:
    DRM:		@3@
    RANDR:		@4@
    VidMode:		@5@
    Quartz (macOS):	@6@
    WinGDI (Windows):	@7@

    Location providers:
    Geoclue2:			@8@
    CoreLocation (macOS):	@9@

    GUI:		@10@
    Ubuntu icons:	@11@
    systemd units:	@12@ @13@
    AppArmor profile:   @14@
'''.format(proj_name, proj_version, get_option('prefix'),
  config_h.has('ENABLE_DRM') ? 'yes' : 'no',
  config_h.has('ENABLE_RANDR') ? 'yes' : 'no',
  config_h.has('ENABLE_VIDMODE') ? 'yes' : 'no',
  config_h.has('ENABLE_QUARTZ') ? 'yes' : 'no',
  config_h.has('ENABLE_WINGDI') ? 'yes' : 'no',
  config_h.has('ENABLE_GEOCLUE2') ? 'yes' : 'no',
  config_h.has('ENABLE_CORELOCATION') ? 'yes' : 'no',
  config_build.has('ENABLE_GUI') ? 'yes' : 'no',
  config_build.has('ENABLE_UBUNTU') ? 'yes' : 'no',
  config_build.has('ENABLE_SYSTEMD') ? 'yes' : 'no', systemd_userunitdir,
  config_build.has('ENABLE_APPARMOR') ? 'yes' : 'no',
))
