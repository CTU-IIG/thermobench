project('libev', 'c',
        version: '4.31')

cc = meson.get_compiler('c')
libm_dep = cc.find_library('m')

libev = library(
  'ev',
  [ 'ev.c', 'event.c' ],
  version : '4.0.0',
  install : true,
  dependencies : [ libm_dep ],
)

libev_dep = declare_dependency(
  link_with : libev,
  include_directories : include_directories('.'),
)
pod2man = find_program('pod2man')
if pod2man.found()
  custom_target(
    'man',
    output : 'ev.3',
    input : 'ev.pod',
    command : [ pod2man, '-n', 'LIBEV', '-r', 'libev-@0@'.format(meson.project_version()),
                '-c', 'libev - high performance full featured event loop', '-s3',
                '@INPUT@', '@OUTPUT@'
              ]
  )
endif



install_headers('ev.h', 'ev++.h', 'event.h')
install_man('ev.3')

cdata = configuration_data()
check_headers = [
  ['HAVE_INTTYPES_H',       'inttypes.h'],
  ['HAVE_DLFCN_H',          'dlfcn.h'],
  ['HAVE_INTTYPES_H',       'inttypes.h'],
  ['HAVE_LINUX_AIO_ABI_H',  'linux/aio_abi.h'],
  ['HAVE_LINUX_FS_H',       'linux/fs.h'],
  ['HAVE_MEMORY_H',         'memory.h'],
  ['HAVE_POLL_H',           'poll.h'],
  ['HAVE_PORT_H',           'port.h'],
  ['HAVE_STDINT_H',         'stdint.h'],
  ['HAVE_STDLIB_H',         'stdlib.h'],
  ['HAVE_STRINGS_H',        'strings.h'],
  ['HAVE_STRING_H',         'string.h'],
  ['HAVE_SYS_EPOLL_H',      'sys/epoll.h'],
  ['HAVE_SYS_EVENTFD_H',    'sys/eventfd.h'],
  ['HAVE_SYS_EVENT_H',      'sys/event.h'],
  ['HAVE_SYS_INOTIFY_H',    'sys/inotify.h'],
  ['HAVE_SYS_SELECT_H',     'sys/select.h'],
  ['HAVE_SYS_SIGNALFD_H',   'sys/signalfd.h'],
  ['HAVE_SYS_STAT_H',       'sys/stat.h'],
  ['HAVE_SYS_TIMERFD_H',    'sys/timerfd.h'],
  ['HAVE_SYS_TYPES_H',      'sys/types.h'],
  ['HAVE_UNISTD_H',         'unistd.h'],
]

foreach h : check_headers
  if cc.has_header(h.get(1))
    cdata.set(h.get(0), 1)
  endif
endforeach

librt = cc.find_library('rt', required : false)
cdata.set10('HAVE_LIBRT', librt.found())

cdata.set10('HAVE_KERNEL_RWF_T',
            cc.has_type('__kernel_rwf_t', prefix : '#include <linux/fs.h>'))

check_functions = [
 ['HAVE_CLOCK_GETTIME', 'clock_gettime', '#include<time.h>'],
 ['HAVE_EPOLL_CTL',     'epoll_ctl',     '#include<sys/epoll.h>'],
 ['HAVE_EVENTFD',       'eventfd',       '#include<sys/eventfd.h>'],
 ['HAVE_FLOOR',         'floor',         '#include<math.h>', libm_dep],
 ['HAVE_INOTIFY_INIT',  'inotify_init',  '#include<sys/inotify.h>'],
 ['HAVE_KQUEUE',        'kqueue',        '#include<sys/event.h>'],
 ['HAVE_NANOSLEEP',     'nanosleep',     '#include<time.h>'],
 ['HAVE_POLL',          'poll',          '#include<poll.h>'],
 ['HAVE_SELECT',        'select',        '#include<sys/select.h>'],
 ['HAVE_SIGNALFD',      'signalfd',      '#include<sys/signalfd.h>']
# TODO: check token ['HAVE_CLOCK_SYSCALL']
# check token ['HAVE_PORT_CREATE']
]

foreach f : check_functions
  if cc.has_function(f.get(1), prefix : f.get(2), dependencies : f.get(3, []))
    cdata.set(f.get(0), 1)
  endif
endforeach

cdata.set_quoted('PACKAGE', meson.project_name())
cdata.set_quoted('PACKAGE_NAME', meson.project_name())
cdata.set_quoted('PACKAGE_VERSION', meson.project_version())
cdata.set_quoted('PACKAGE_BUGREPORT', '')
cdata.set_quoted('PACKAGE_STRING', '@0@ @1@'.format(meson.project_name(), meson.project_version()))
cdata.set('VERSION', meson.project_version())

configure_file(input : 'config.h.meson',
  output : 'config.h',
  configuration : cdata)
