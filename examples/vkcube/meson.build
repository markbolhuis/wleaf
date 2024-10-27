
dep_vulkan = dependency('vulkan', version : '>=1.0.0')
dep_glm    = dependency('cglm', version : '>=0.0.1')

glslc_bin = find_program('glslc')

vkcube_shaders_src = [
  'vkcube.frag',
  'vkcube.vert',
]

foreach shader : vkcube_shaders_src
  custom_target(
    shader.underscorify(),
    input : shader,
    output : '@PLAINNAME@.spv',
    command : [glslc_bin, '@INPUT@', '-o', '@OUTPUT@'],
    build_by_default : true,
  )
endforeach

executable('vkcube',
  'vkcube.c',
  dependencies: [
    dep_vulkan,
    dep_wlf,
    dep_glm,
    lib_m,
  ],
)

