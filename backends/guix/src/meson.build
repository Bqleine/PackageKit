guile_load_path = get_option('guix_guile_load_path')
guile_load_compiled_path = get_option('guix_guile_load_compiled_path')

shared_module(
	'pk_backend_guix',
	'pk-backend-guix.c',
    'pk-guile-interface.c',
	include_directories: packagekit_src_include,
	dependencies: [
		packagekit_glib2_dep,
		gmodule_dep,
		libguile_dep
	],
	c_args: [
		'-DG_LOG_DOMAIN="PackageKit-Guix"',
        '-DGUILE_LOAD_PATH="' + guile_load_path + '"',
        '-DGUILE_LOAD_COMPILED_PATH="' + guile_load_compiled_path + '"',
	],
	install: true,
	install_dir: pk_plugin_dir,
)
