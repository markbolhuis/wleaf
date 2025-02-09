dep_wl_client = dependency('wayland-client', version: '>= 1.21.0')
dep_wl_cursor = dependency('wayland-cursor', version: '>= 1.21.0')
dep_wl_egl    = dependency('wayland-egl', version: '>= 1.21.0')
dep_wl_protos = dependency('wayland-protocols', version: '>=1.36')
dep_xkbcommon = dependency('xkbcommon', version : '>= 1.4.0')

wl_scanner = find_program('wayland-scanner', version : dep_wl_client.version())

wl_proto_root = dep_wl_protos.get_variable('pkgdatadir')

wl_protos = [
    [ 'viewporter', 'stable' ],
    [ 'fractional-scale', 'staging', 1 ],
    [ 'input-timestamps', 'unstable', 1 ],
    [ 'relative-pointer', 'unstable', 1 ],
    [ 'pointer-constraints', 'unstable', 1 ],
    [ 'pointer-gestures', 'unstable', 1 ],
    [ 'text-input', 'unstable', 3 ],
    [ 'keyboard-shortcuts-inhibit', 'unstable', 1 ],
    [ 'idle-inhibit', 'unstable', 1 ],
    [ 'content-type', 'staging', 1 ],
    [ 'xdg-shell', 'stable' ],
    [ 'xdg-output', 'unstable', 1 ],
    [ 'xdg-decoration', 'unstable', 1 ],
    [ 'xdg-dialog', 'staging', 1 ],
    [ 'ext-idle-notify', 'staging', 1 ],
    [ 'single-pixel-buffer', 'staging', 1 ],
    [ 'alpha-modifier', 'staging', 1 ],
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
)

src_proto = []
hdr_proto = []

foreach p : wl_protos
  p_name = p.get(0)
  p_state = p.get(1)

  if p_state == 'stable'
    if p.length() == 3
      p_file = '@0@-v@1@'.format(p_name, p.get(2))
    else
      p_file = p_name
    endif
  elif p_state == 'staging'
    p_file = '@0@-v@1@'.format(p_name, p.get(2))
  elif p_state == 'unstable'
    p_file = '@0@-unstable-v@1@'.format(p_name, p.get(2))
  else
    error(f'Unknown protocol type: @p_state@')
  endif

  src_proto += custom_target(f'@p_name@-code',
    input : join_paths(wl_proto_root, p_state, p_name, f'@p_file@.xml'),
    output : f'@p_file@-protocol.c',
    command : [wl_scanner, 'private-code', '@INPUT@', '@OUTPUT@'])

  hdr_proto += custom_target(f'@p_name@-client-header',
    input : join_paths(wl_proto_root, p_state, p_name, f'@p_file@.xml'),
    output : f'@p_file@-client-protocol.h',
    command : [wl_scanner, 'client-header', '--include-core-only', '@INPUT@', '@OUTPUT@'])

endforeach

lib_wlf = library('wleaf',
  src_wlf,
  src_proto,
  hdr_proto,
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
