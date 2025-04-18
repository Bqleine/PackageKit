project('PackageKit', 'c',
  version : '1.3.1',
  license : 'LGPL-2.1+',
  meson_version : '>=0.50',
  default_options : ['warning_level=2', 'c_std=c99'],
)

gnome = import('gnome')
i18n = import('i18n')
pkg = import('pkgconfig')

glib_dep = dependency('glib-2.0', version: '>=2.62')
gobject_dep = dependency('gobject-2.0')
gio_dep = dependency('gio-2.0')
gio_unix_dep = dependency('gio-unix-2.0', version: '>=2.16.1')
gmodule_dep = dependency('gmodule-2.0', version: '>=2.16.1')
sqlite3_dep = dependency('sqlite3')
polkit_dep = dependency('polkit-gobject-1', version: '>=0.98')
if polkit_dep.version().version_compare('>=0.114')
  add_project_arguments ('-DHAVE_POLKIT_0_114=1', language: 'c')
endif

libsystemd = []
if get_option('systemd')
  libsystemd = dependency('libsystemd', version: '>=213')
  systemd_system_unit_dir = get_option('systemdsystemunitdir')
  systemd_user_unit_dir = get_option('systemduserunitdir')
  if systemd_system_unit_dir == '' or systemd_user_unit_dir == ''
    systemd = dependency('systemd')
  endif
  if systemd_system_unit_dir == ''
    systemd_system_unit_dir = systemd.get_pkgconfig_variable('systemdsystemunitdir')
  endif
  if systemd_user_unit_dir == ''
    systemd_user_unit_dir = systemd.get_pkgconfig_variable('systemduserunitdir')
  endif

  add_project_arguments ('-DHAVE_SYSTEMD_SD_DAEMON_H=1', language: 'c')
  add_project_arguments ('-DHAVE_SYSTEMD_SD_LOGIN_H=1', language: 'c')
  add_project_arguments ('-DHAVE_SYSTEMD_SD_JOURNAL_H=1', language: 'c')
endif

elogind = []
if get_option('elogind')
  elogind = dependency('libelogind', version: '>=229.4')
  add_project_arguments ('-DHAVE_SYSTEMD_SD_LOGIN_H=1', language: 'c')
  add_project_arguments ('-DHAVE_SYSTEMD_SD_JOURNAL_H=1', language: 'c')
endif

if get_option('local_checkout')
  add_project_arguments ('-DPK_BUILD_LOCAL=1', language: 'c')
endif

if get_option('daemon_tests')
  add_project_arguments ('-DPK_ENABLE_DAEMON_TESTS=1', language: 'c')
endif

if dependency('ply-boot-client', version: '>=0.9.5', required: false).found()
  add_project_arguments ('-DPLYMOUTH_0_9_5=1', language: 'c')
endif

# Ensure functions like realpath(3) and other "default" functions are available
add_project_arguments ('-D_DEFAULT_SOURCE', language: 'c')

# Avoid g_simple_async_result deprecation warnings in glib 2.46+
add_project_arguments ('-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_44', language: 'c')

# allow the daemon to include library files directly
add_project_arguments ('-DPK_COMPILATION', language: ['c', 'cpp'])

conf = configuration_data()
conf.set_quoted('DATADIR', join_paths(get_option('prefix'), get_option('datadir')))
conf.set_quoted('LIBDIR', join_paths(get_option('prefix'), get_option('libdir')))
conf.set_quoted('SYSCONFDIR', get_option('sysconfdir'))

conf.set_quoted('PROJECT_NAME', meson.project_name())
conf.set_quoted('PROJECT_VERSION', meson.project_version())

cc = meson.get_compiler('c')
if cc.has_function('setpriority')
  conf.set('HAVE_SETPRIORITY', '1')
endif
if cc.has_function('clearenv')
  conf.set('HAVE_CLEARENV', '1')
endif
if cc.has_header('unistd.h')
  conf.set('HAVE_UNISTD_H', '1')
endif

config_header = configure_file(
  output: 'config.h',
  configuration: conf,
)

config_dep = declare_dependency(
  sources: config_header,
  include_directories: include_directories('.')
)

pk_db_dir = join_paths(get_option('localstatedir'), 'lib', 'PackageKit')
local_state_dir = get_option('localstatedir')
test_data_dir = join_paths(meson.source_root(), 'tests', 'data')
package_data_dir = get_option('datadir')
package_locale_dir = join_paths(get_option('prefix'), get_option('datadir'), 'locale')
pk_plugin_dir = join_paths(get_option('prefix'), get_option('libdir'), 'packagekit-backend')

# default compiler flags for C and C++
add_project_arguments(
  '-Wall',
  '-Wcast-align',
  '-Wno-uninitialized',
  '-Werror=missing-declarations',
  '-Wredundant-decls',
  '-Wpointer-arith',
  '-Wcast-align',
  '-Wwrite-strings',
  '-Winit-self',
  '-Wreturn-type',
  '-Wformat-nonliteral',
  '-Wformat-security',
  '-Wmissing-include-dirs',
  '-Wmissing-format-attribute',
  '-Wclobbered',
  '-Wempty-body',
  '-Wignored-qualifiers',
  '-Wsign-compare',
  '-Wtype-limits',
  '-Wuninitialized',
  '-Wno-unused-parameter',
  language: ['c', 'cpp']
)

# default compiler flags for C only
add_project_arguments(
  '-Werror=shadow',
  '-Werror=empty-body',
  '-Werror=strict-prototypes',
  '-Werror=missing-prototypes',
  '-Werror=implicit-function-declaration',
  '-Werror=return-type',
  '-Werror=int-conversion',
  '-Werror=incompatible-pointer-types',
  '-Werror=misleading-indentation',
  '-Werror=missing-include-dirs',
  '-Werror=declaration-after-statement',
  '-Werror=format-security',
  '-Waggregate-return',
  '-Wno-strict-aliasing',
  '-Winline',
  '-Wmissing-parameter-type',
  '-Woverride-init',
  language: 'c'
)

subdir('po')
subdir('policy')
subdir('etc')
subdir('data')
subdir('lib')
subdir('src')
subdir('client')
subdir('backends')
subdir('contrib')
subdir('docs')
