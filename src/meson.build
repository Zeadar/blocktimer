cc = meson.get_compiler('c')

# Try libelogind first
elogind_dep = dependency('libelogind', required: false)

if elogind_dep.found()
  sdbus_dep = elogind_dep
  # add_project_arguments('-DHAVE_ELOGIND', language: 'c')
else
  systemd_dep = dependency('libsystemd', required: false)
  if systemd_dep.found()
    sdbus_dep = systemd_dep
    # add_project_arguments('-DHAVE_SYSTEMD', language: 'c')
  else
    error('Neither libelogind nor libsystemd found')
  endif
endif

# pthread_dep = dependency('threads')

sources = [
  'addr.c',
  'handle_errors.c',
  'parse_config.c',
  'waitforwakeup.c',
  'fetch_addresses.c',
  'main.c',
  'libmemhandle/slice.c',
  'libmemhandle/sarray.c',
  'libmemhandle/hashy.c',
]

executable(
  'blocktimer',
  sources,
  dependencies: [sdbus_dep],
  install: true
)

