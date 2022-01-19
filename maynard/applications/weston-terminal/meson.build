maynard_terminal_ini = configure_file(
    input: 'weston-terminal.desktop.in',
    output: 'weston-terminal.desktop',
    configuration: config_maynard,
    install: true,
    install_dir: join_paths(dir_sysconfdir, '/usr/share/applications/'),
)

maynard_terminal_icon48 = install_data(
    sources: 'weston-terminal-icon.png',
    install: true,
    install_dir: join_paths(dir_sysconfdir, '/usr/share/icons/hicolor/48x48/apps/'),
)