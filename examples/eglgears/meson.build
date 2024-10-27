dep_opengl = dependency('opengl')
dep_egl    = dependency('egl')

executable('eglgears',
  'eglgears.c',
  dependencies : [
    dep_egl,
    dep_opengl,
    dep_wlf,
    lib_m,
  ],
)
