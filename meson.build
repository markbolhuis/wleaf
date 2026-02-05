project('wleaf', 'c',
  version : '0.0.1',
  license : 'LGPLv3',
  default_options : [
    'c_std=gnu23',
    'warning_level=3',
    'werror=true',
  ],
  meson_version : '>= 1.8.0',
)

mod_pkg = import('pkgconfig')

mod_fs = import('fs')
relative_dir = mod_fs.relative_to(meson.current_source_dir(), meson.global_build_root()) + '/'
message(relative_dir)

add_project_arguments(
  '-fmacro-prefix-map=@0@='.format(relative_dir),
  '-Wno-unused-parameter',
  language : 'c',
)

inc_wlf = include_directories('include')

subdir('include')
subdir('src')
if get_option('examples')
  subdir('examples')
endif
