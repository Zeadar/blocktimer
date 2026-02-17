project(
  'blocktimer',
  'c',
  version: '1.0.0',
  default_options: [
    'warning_level=3',
    'c_std=gnu11'
  ]
)

subdir('src')

sysconfdir = get_option('sysconfdir')

install_data(
  'block.conf',
  install_dir: sysconfdir / 'blocktimer',
  install_mode: 'rw-r--r--'
)

if get_option('openrc')
  install_data(
    'open-rc/blocktimer',
    install_dir: sysconfdir / 'init.d',
    install_mode: 'rwxr-xr-x'
  )
endif

if get_option('systemd')
  systemd_dep = dependency('systemd', required: false)

  if systemd_dep.found()
    systemd_system_unit_dir = systemd_dep.get_variable(
      pkgconfig: 'systemdsystemunitdir',
      default_value: '/usr/lib/systemd/system'
    )

    install_data(
      'systemd/blocktimer.service',
      install_dir: systemd_system_unit_dir
    )
  endif
endif
