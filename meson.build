project('alerttomaild', 'cpp', 'c',
    default_options: [
      'buildtype=debugoptimized',
      'cpp_std=c++17',
    ],
)

#libsystemd_pkg = dependency('libsystemd')
#root_inc = include_directories('/usr/local/lib')

libsdbusplus = dependency('sdbusplus')


executable('alerttomaild',
           'main.cpp',
           'conf.cpp',
           'daemon.cpp',
           'buslisten.cpp',
           dependencies: libsdbusplus,
           install : true)

