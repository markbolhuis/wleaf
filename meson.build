project('wleaf', 'c',
  version : '0.0.1',
  license : 'LGPLv3',
  default_options : [
    'c_std=gnu23',
    'warning_level=3',
    'werror=true',
  ],
  meson_version : '>= 0.63.0',
)

add_project_arguments(
  '-Wno-unused-parameter',
  language : 'c',
)

inc_wlf = include_directories('include')

subdir('include')
subdir('src')
if get_option('examples')
  subdir('examples')
endif
