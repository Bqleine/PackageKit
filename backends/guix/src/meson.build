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
	],
	install: true,
	install_dir: pk_plugin_dir,
)
