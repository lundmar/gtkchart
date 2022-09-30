cc = meson.get_compiler('c')
libm_dep = cc.find_library('m', required : false)

libgtk_dep = dependency('gtk4', version: '>= 4.5.0', required: true,
                         fallback : ['gtk', 'libgtk_dep'],
                         default_options: ['introspection=disabled',
                                           'demos=false',
                                           'build-examples=false',
                                           'build-tests=false'])

libadwaita_dep = dependency('libadwaita-1', version: '>= 1.0.1', required: true,
                             fallback : ['libadwaita', 'libadwaita_dep'],
                             default_options: ['introspection=disabled',
                                               'examples=false',
                                               'tests=false'])

demo_deps = [libm_dep, libgtk_dep, libadwaita_dep]

executable('gtkchart-demo',
           'gtkchart-demo.c',
            dependencies: demo_deps,
            include_directories: include_directories('../src'),
            link_with: libgtkchart,
            install: false,
)
