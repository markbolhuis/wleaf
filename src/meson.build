dep_wl_client = dependency('wayland-client', version: '>= 1.21.0')
dep_wl_cursor = dependency('wayland-cursor', version: '>= 1.21.0')
dep_wl_egl    = dependency('wayland-egl', version: '>= 1.21.0')
dep_xkbcommon = dependency('xkbcommon', version : '>= 1.4.0')

wl_mod = import('wayland')

wl_protos = [
    wl_mod.find_protocol('viewporter'),
    wl_mod.find_protocol('fractional-scale', state : 'staging', version : 1 ),
    wl_mod.find_protocol('input-timestamps', state : 'unstable', version : 1 ),
    wl_mod.find_protocol('relative-pointer', state : 'unstable', version : 1 ),
    wl_mod.find_protocol('pointer-constraints', state : 'unstable', version : 1 ),
    wl_mod.find_protocol('pointer-gestures', state : 'unstable', version : 1 ),
    wl_mod.find_protocol('text-input', state : 'unstable', version : 3 ),
    wl_mod.find_protocol('keyboard-shortcuts-inhibit', state : 'unstable', version : 1 ),
    wl_mod.find_protocol('idle-inhibit', state : 'unstable', version : 1 ),
    wl_mod.find_protocol('content-type', state : 'staging', version : 1 ),
    wl_mod.find_protocol('xdg-shell'),
    wl_mod.find_protocol('xdg-output', state : 'unstable', version : 1 ),
    wl_mod.find_protocol('xdg-decoration', state : 'unstable', version : 1 ),
    wl_mod.find_protocol('xdg-dialog', state : 'staging', version : 1 ),
    wl_mod.find_protocol('ext-idle-notify', state : 'staging', version : 1 ),
    wl_mod.find_protocol('single-pixel-buffer', state : 'staging', version : 1 ),
    wl_mod.find_protocol('alpha-modifier', state : 'staging', version : 1 ),
]

src_wlf = files(
  'context.c',
  'input.c',
  'surface.c',
  'toplevel.c',
  'popup.c',
  'output.c',
  'egl.c',
  'vulkan.c',
  'log.c',
)

lib_wlf = library('wleaf',
  src_wlf,
  wl_mod.scan_xml(wl_protos),
  include_directories : inc_wlf,
  install : true,
  dependencies : [
    dep_wl_client,
    dep_wl_cursor,
    dep_wl_egl,
    dep_xkbcommon,
  ],
)

dep_wlf = declare_dependency(
  link_with : lib_wlf,
  include_directories : inc_wlf
)

meson.override_dependency('wleaf', dep_wlf)

mod_pkg.generate(lib_wlf, subdirs : 'wleaf-0.0')
