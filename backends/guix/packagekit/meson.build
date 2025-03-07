guild = libguile_dep.get_pkgconfig_variable('guild')
guile = libguile_dep.get_pkgconfig_variable('guile')

guile_output = join_paths(get_option('prefix'), get_option('datadir'), 'guile', 'site', '3.0')
guile_compiled_output = join_paths(get_option('prefix'), get_option('libdir'), 'guile', '3.0', 'site-ccache')

guile_compile = [
  guild,
  'compile',
  '-W3',
  '-L', meson.current_source_dir() / '..',
  '-o',
  '@OUTPUT@',
  '@INPUT@',
]

module_sources = [
  'pk-guile-interface.scm',
  'pk-profile.scm',
  'pk-id.scm',
]

install_data(module_sources, install_dir : guile_output / 'packagekit')

foreach src : module_sources
  obj = src.split('.')[0] + '.go'
  custom_target(
    obj,
    command: guile_compile,
    input: src,
    install: true,
    install_dir: join_paths(guile_compiled_output, 'packagekit'),
    output: obj
  )
endforeach
