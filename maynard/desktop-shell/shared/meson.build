srcs_libshared = [
    'config-parser.c',
    'option-parser.c',
    'file-util.c',
    'os-compatibility.c',
    'xalloc.c',
]
deps_libshared = dep_wayland_client

lib_libshared = static_library(
    'shared',
    srcs_libshared,
    include_directories: include_directories('..', '../..'),
    dependencies: deps_libshared,
    pic: true,
    install: false
)
dep_libshared = declare_dependency(
    link_with: lib_libshared,
    dependencies: deps_libshared
)
