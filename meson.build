project('alerttomail', 'cpp', 'c',
    default_options: [
      'buildtype=debugoptimized',
      'cpp_std=c++17',
    ],
)

#libsystemd_pkg = dependency('libsystemd')
#root_inc = include_directories('include')


executable('alerttomaild',
           'main.cpp',
           'conf.cpp',
           #dependencies: sdbusplus_dep,
           install : true)

