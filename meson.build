# Dynamic-Larpland: a Dynamic Island for Wayland compositors.
# The UI/render/Wayland library in lib/ is Noctalia's noctalia-ui (MIT), vendored verbatim.
project('dynamic-larpland', 'cpp', 'c',
  version: '0.1.0',
  meson_version: '>=1.1',
  default_options: ['cpp_std=c++23', 'warning_level=2', 'buildtype=debugoptimized'],
)

noctalia_ui_build_codecs = true
subdir('lib')

wayland_scanner = find_program('wayland-scanner')
# wlr-layer-shell names a parameter 'namespace' (a C++ keyword): rename it in the generated header.
layer_shell_h = custom_target('wlr-layer-shell-client-header',
  input: 'protocols/wlr-layer-shell-unstable-v1.xml',
  output: 'wlr-layer-shell-unstable-v1-client-protocol.h',
  command: ['sh', '-c', '"$1" client-header "$2" "$3" && sed -i s/namespace/name_space/g "$3"', 'sh',
            wayland_scanner, '@INPUT@', '@OUTPUT@'])
layer_shell_c = custom_target('wlr-layer-shell-client-source',
  input: 'protocols/wlr-layer-shell-unstable-v1.xml',
  output: 'wlr-layer-shell-unstable-v1-client-protocol.c',
  command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'], depends: layer_shell_h)
protocol_sources = [layer_shell_c]
protocol_headers = [layer_shell_h]

sources = files(
  'src/main.cpp',
  'src/app/application.cpp',
  'src/app/library.cpp',
  'src/app/main_loop.cpp',
  'src/config/config.cpp',
  'src/ipc/ipc.cpp',
  'src/island/island.cpp',
  'src/wayland/layer_shell.cpp',
  'src/wayland/layer_surface.cpp',
)

executable('dynamic-larpland',
  sources + protocol_sources + protocol_headers,
  dependencies: [noctalia_ui_dep, noctalia_ui_codecs_dep, dependency('tomlplusplus')],
  include_directories: include_directories('src'),
  # shortcut: assets are read from the source tree, fine for development; install them for packaging (milestone 7).
  cpp_args: [
    '-DDL_APP_ASSETS_DIR="' + (meson.current_source_dir() / 'assets') + '"',
    '-DDL_LIB_ASSETS_DIR="' + (meson.current_source_dir() / 'lib' / 'assets') + '"',
  ],
  install: true,
)
