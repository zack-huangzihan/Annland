subdir('shared')

config_h.set_quoted('WESTON_SHELL_CLIENT', 'maynard')

srcs_shell_desktop = [
    'shell.c',
    'exposay.c',
    'input-panel.c',
    'shared/matrix.c',
    weston_desktop_shell_server_protocol_h,
    weston_desktop_shell_protocol_c,
    input_method_unstable_v1_server_protocol_h,
    input_method_unstable_v1_protocol_c,
]
deps_shell_desktop = [
    dep_pixman,
    dep_libm,
    dep_libshared,
    dep_libweston,
    dep_wayland_client,
    dep_wayland_server,
    dep_weston,
]
plugin_shell_desktop = shared_library(
    'maynard-desktop-shell',
    srcs_shell_desktop,
    include_directories: include_directories('..', 'shared'),
    dependencies: deps_shell_desktop,
    name_prefix: '',
    install: true,
    install_dir: dir_module_weston
)
