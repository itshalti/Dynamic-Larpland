# Dynamic-Larpland: a Dynamic Island for Wayland compositors.
# The UI/render/Wayland library in lib/ is Noctalia's noctalia-ui (MIT), vendored verbatim.
project('dynamic-larpland', 'cpp', 'c',
  version: '0.1.0',
  meson_version: '>=1.1',
  default_options: ['cpp_std=c++23', 'warning_level=2', 'buildtype=debugoptimized'],
)

noctalia_ui_build_codecs = true
subdir('lib')

# Vendored Material Color Utilities (Google, Apache-2.0), as in Noctalia: palette generation from images.
mcu_inc = include_directories('third_party/material_color_utilities', is_system: true)
mcu_lib = static_library('material_color_utilities',
  files(
    'third_party/material_color_utilities/cpp/blend/blend.cc',
    'third_party/material_color_utilities/cpp/cam/cam.cc',
    'third_party/material_color_utilities/cpp/cam/hct.cc',
    'third_party/material_color_utilities/cpp/cam/hct_solver.cc',
    'third_party/material_color_utilities/cpp/cam/viewing_conditions.cc',
    'third_party/material_color_utilities/cpp/contrast/contrast.cc',
    'third_party/material_color_utilities/cpp/dislike/dislike.cc',
    'third_party/material_color_utilities/cpp/dynamiccolor/dynamic_color.cc',
    'third_party/material_color_utilities/cpp/dynamiccolor/dynamic_scheme.cc',
    'third_party/material_color_utilities/cpp/dynamiccolor/material_dynamic_colors.cc',
    'third_party/material_color_utilities/cpp/palettes/tones.cc',
    'third_party/material_color_utilities/cpp/quantize/celebi.cc',
    'third_party/material_color_utilities/cpp/quantize/lab.cc',
    'third_party/material_color_utilities/cpp/quantize/wsmeans.cc',
    'third_party/material_color_utilities/cpp/quantize/wu.cc',
    'third_party/material_color_utilities/cpp/scheme/scheme_content.cc',
    'third_party/material_color_utilities/cpp/scheme/scheme_expressive.cc',
    'third_party/material_color_utilities/cpp/scheme/scheme_fidelity.cc',
    'third_party/material_color_utilities/cpp/scheme/scheme_fruit_salad.cc',
    'third_party/material_color_utilities/cpp/scheme/scheme_monochrome.cc',
    'third_party/material_color_utilities/cpp/scheme/scheme_neutral.cc',
    'third_party/material_color_utilities/cpp/scheme/scheme_rainbow.cc',
    'third_party/material_color_utilities/cpp/scheme/scheme_tonal_spot.cc',
    'third_party/material_color_utilities/cpp/scheme/scheme_vibrant.cc',
    'third_party/material_color_utilities/cpp/score/score.cc',
    'third_party/material_color_utilities/cpp/temperature/temperature_cache.cc',
    'third_party/material_color_utilities/cpp/utils/utils.cc',
  ),
  include_directories: mcu_inc,
  cpp_args: ['-Wno-pedantic', '-Wno-conversion', '-Wno-shadow', '-Wno-unused-parameter'],
  override_options: ['warning_level=0'],
)
mcu_dep = declare_dependency(link_whole: mcu_lib, include_directories: mcu_inc)

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
  'src/config/atomic_file.cpp',
  'src/config/config.cpp',
  'src/ipc/ipc.cpp',
  'src/island/island.cpp',
  'src/island/wallpaper_page.cpp',
  'src/services/wallpaper_service.cpp',
  'src/system/process.cpp',
  'src/theme/builtin_templates.cpp',
  'src/theme/color.cpp',
  'src/theme/contrast.cpp',
  'src/theme/custom_schemes.cpp',
  'src/theme/fixed_palette.cpp',
  'src/theme/hook_runner.cpp',
  'src/theme/image_loader.cpp',
  'src/theme/json_output.cpp',
  'src/theme/kde_color_scheme.cpp',
  'src/theme/m3_schemes.cpp',
  'src/theme/palette_generator.cpp',
  'src/theme/palette_transform.cpp',
  'src/theme/scheme.cpp',
  'src/theme/template_engine.cpp',
  'src/theme/theme_cli.cpp',
  'src/wallpaper/background.cpp',
  'src/wayland/layer_shell.cpp',
  'src/wayland/layer_surface.cpp',
)

executable('dynamic-larpland',
  sources + protocol_sources + protocol_headers,
  dependencies: [noctalia_ui_dep, noctalia_ui_codecs_dep, dependency('tomlplusplus'), dependency('sdbus-c++'), mcu_dep],
  include_directories: include_directories('src'),
  # shortcut: assets are read from the source tree, fine for development; install them for packaging (milestone 7).
  cpp_args: [
    '-DDL_APP_ASSETS_DIR="' + (meson.current_source_dir() / 'assets') + '"',
    '-DDL_LIB_ASSETS_DIR="' + (meson.current_source_dir() / 'lib' / 'assets') + '"',
  ],
  install: true,
)
