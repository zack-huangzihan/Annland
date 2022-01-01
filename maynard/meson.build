project('maynard',
    'c',
    version: '0.2.0+git',
    default_options: [
        'warning_level=2',
        'c_std=gnu99',
        'b_lundef=false',
    ],
    license: [ 'MIT/Expat', 'GPL2+' ]
)

version_maynard = meson.project_version()

dir_prefix = get_option('prefix')
dir_bin = join_paths(dir_prefix, get_option('bindir'))
dir_data = join_paths(dir_prefix, get_option('datadir'))
dir_lib = join_paths(dir_prefix, get_option('libdir'))
dir_libexec = join_paths(dir_prefix, get_option('libexecdir'))
dir_module_weston = join_paths(dir_lib, 'weston')
dir_sysconfdir = join_paths(dir_prefix, get_option('sysconfdir'))

gnome = import ('gnome')
i18n = import ('i18n')
pkgconfig = import('pkgconfig')

config_h = configuration_data()

cc = meson.get_compiler('c')

global_args = []
global_args_maybe = [
    '-Wno-unused-parameter',
    '-Wno-shift-negative-value', # required due to Pixman
    '-Wno-missing-field-initializers',
    '-fvisibility=hidden',
]
foreach a : global_args_maybe
    if cc.has_argument(a)
        global_args += a
    endif
endforeach
add_global_arguments(global_args, language: 'c')

config_h.set('_GNU_SOURCE', '1')
config_h.set('_ALL_SOURCE', '1')

config_h.set_quoted('PACKAGE_STRING', 'weston @0@'.format(version_maynard))
config_h.set_quoted('PACKAGE_VERSION', version_maynard)
config_h.set_quoted('VERSION', version_maynard)

config_h.set_quoted('BINDIR', dir_bin)
config_h.set_quoted('DATADIR', dir_data)
config_h.set_quoted('LIBEXECDIR', dir_libexec)

gettext_package = 'maynard'
add_project_arguments('-DGETTEXT_PACKAGE=' + gettext_package, language: 'c')

dep_alsa = dependency('alsa')
dep_gnome_desktop = dependency('gnome-desktop-3.0')
dep_gtk = dependency('gtk+-3.0', version: '>= 3.16')
dep_gtk_wayland = dependency('gtk+-wayland-3.0')
dep_libm = cc.find_library('m')
dep_libweston = dependency('libweston-9')
dep_pixman = dependency('pixman-1', version: '>= 0.25.2')
dep_wayland_server = dependency('wayland-server', version: '>= 1.12.0')
dep_wayland_client = dependency('wayland-client', version: '>= 1.12.0')
dep_weston = dependency('weston')

dep_canterbury = dependency('canterbury-platform-0', required: get_option('enable-canterbury'))
if dep_canterbury.found()
    config_h.set('HAVE_CANTERBURY', '1')
endif

dep_gnome_menu = dependency('libgnome-menu-3.0', required: get_option('enable-gnome-menu'))
if dep_gnome_menu.found()
    config_h.set('HAVE_GNOME_MENU', '1')
endif

config_maynard = configuration_data()
config_maynard.set('prefix', dir_prefix)
config_maynard.set('bindir', dir_bin)
config_maynard.set('libdir', dir_lib)
config_maynard.set('datadir', dir_data)
config_maynard.set('libexecdir', dir_libexec)
maynard_script = configure_file(
    input: 'maynard.in',
    output: 'maynard',
    configuration: config_maynard,
    install: true,
    install_dir: dir_bin,
    install_mode: 'rwxr-xr-x'
)

maynard_ini = configure_file(
    input: 'maynard.ini.in',
    output: 'maynard.ini',
    configuration: config_maynard,
    install: true,
    install_dir: join_paths(dir_data, 'maynard'),
)

maynard_menu_ini = configure_file(
    input: 'maynard-applications.menu.in',
    output: 'maynard-applications.menu',
    configuration: config_maynard,
    install: true,
    install_dir: join_paths(dir_sysconfdir, 'xdg/menus/'),
)

subdir('data')
subdir('protocol')
subdir('desktop-shell')
subdir('client')
subdir('po')

configure_file(output: 'config.h', install: false, configuration: config_h)
