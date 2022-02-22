libgtkchart_sources = [
  'gtkchart.c',
]

libgtk_dep = dependency('gtk4', version: '>= 4.4.0', required: true,
                        fallback : ['gtk', 'libgtk_dep'],
                        default_options: ['introspection=disabled',
                                          'demos=false',
                                          'build-examples=false',
                                          'build-tests=false'])

libgtkchart_deps = [ libgtk_dep ]

libgtkchart_c_args = [
  '-fvisibility=hidden',
]

libgtkchart = shared_library(
  'gtkchart',
  libgtkchart_sources,
  dependencies: libgtkchart_deps,
  install: true,
  c_args: libgtkchart_c_args,
)

install_headers('gtkchart.h')
