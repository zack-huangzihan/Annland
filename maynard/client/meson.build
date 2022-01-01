resources_maynard = gnome.compile_resources(
    'maynard-resources', 'maynard.gresource.xml',
    source_dir : '.',
    c_name : 'maynard')

srcs_maynard = [
    'maynard.c',
    'app-icon.c',
    'app-icon.h',
    'favorites.c',
    'favorites.h',
    'shell-app-system.c',
    'shell-app-system.h',
    'panel.c',
    'panel.h',
    'launcher.c',
    'launcher.h',
    'indicator.c',
    'indicator.h',
    'clock-indicator.c',
    'clock-indicator.h',
    'volume-indicator.c',
    'volume-indicator.h',
    resources_maynard,
    shell_helper_client_protocol_h,
    shell_helper_protocol_c,
    weston_desktop_shell_client_protocol_h,
    weston_desktop_shell_protocol_c,
]

deps_maynard = [
    dep_alsa,
    dep_canterbury,
    dep_gnome_desktop,
    dep_gnome_menu,
    dep_gtk,
    dep_libm,
    dep_wayland_client,
]

exe_maynard = executable(
    'maynard',
    srcs_maynard,
    include_directories: include_directories('..'),
    dependencies: deps_maynard,
    install_dir: get_option('libexecdir'),
    install: true
)

srcs_shell_helper = [
    'shell-helper.c',
    shell_helper_protocol_c,
    shell_helper_server_protocol_h,
]

deps_shell_helper = [
    dep_libweston,
    dep_pixman,
    dep_wayland_server,
    dep_wayland_client,
]

plugin_shell_helper = shared_library(
    'maynard-shell-helper',
    srcs_shell_helper,
    include_directories: include_directories('..'),
    dependencies: deps_shell_helper,
    name_prefix: '',
    install: true,
    install_dir: dir_module_weston
)
