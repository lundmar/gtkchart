libgtkchart_sources = ['gtkchart.c']

libglib_dep = dependency('glib-2.0', version: '>= 2.70', required: true,
                          fallback : ['glib', 'libglib_dep'],
                          default_options: ['tests=false'])

libgtk_dep = dependency('gtk4', version: '>= 4.4.0', required: true,
                        fallback : ['gtk', 'libgtk_dep'],
                        default_options: ['introspection=disabled',
                                          'demos=false',
                                          'build-examples=false',
                                          'build-tests=false'])

libgtkchart_deps = [libglib_dep, libgtk_dep]

libgtkchart = shared_library('gtkchart',
                              libgtkchart_sources,
                              dependencies: libgtkchart_deps,
                              install: true
)

install_headers('gtkchart.h')
