project('libgtkchart', 'c',
  version: '0.1',
  license: 'BSD-3-Clause',
  meson_version: '>= 0.53.2',
  default_options: [ 'warning_level=2', 'buildtype=release', 'c_std=gnu11', 'optimization=2', 'b_lundef=false' ]
)

subdir('src')

pkg_mod = import('pkgconfig')
pkg_mod.generate(libraries: libgtkchart,
  name: 'libgtkchart',
  filebase: 'libgtkchart',
  description: 'A small chart widget library for GTK4'
)
