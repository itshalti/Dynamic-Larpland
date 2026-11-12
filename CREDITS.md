# Credits

Dynamic-Larpland's native (C++) version is built on code from
[Noctalia](https://github.com/noctalia-dev/noctalia-shell), MIT License, Copyright (c) 2026 noctalia-dev:

- `lib/` — Noctalia's `noctalia-ui` library (renderer, scene graph, UI controls, Wayland transport), vendored
  verbatim. Its license is in `lib/LICENSE.noctalia`.
- `src/wayland/layer_shell.*`, `src/wayland/layer_surface.*` — adapted from Noctalia's `src/shell/wayland/`
  (blur support removed).
- `src/app/main_loop.*`, `src/app/*_poll_source.h` — adapted from Noctalia's `src/app/`.
- `src/theme/` — Noctalia's theme engine (palette generation, Material You and custom schemes, template
  processor, hooks), ported with the Firefox integration and Noctalia's icon resolver removed;
  `theme_cli.cpp` is new.
- `src/system/process.*`, `src/config/atomic_file.*`, `src/config/toml.h` — from Noctalia.
- `src/dbus/session_bus.*`, `src/dbus/mpris/` (Noctalia's own D-Bus control API and IPC hooks removed),
  `src/dbus/tray_service.*`, `src/net/` (HTTP client for remote cover art), `src/system/icon_resolver.*`
  (plus a `DYNAMIC_LARPLAND_ICON_THEME` override), `src/system/freedesktop_key_file.*` — from Noctalia.
- `assets/templates/` — Noctalia's built-in app template catalog, output files renamed from `noctalia.*` to
  `dynamic-larpland.*`.
- `third_party/material_color_utilities/` — Google's Material Color Utilities (Apache-2.0), as vendored by Noctalia.
- `protocols/wlr-layer-shell-unstable-v1.xml` — wlr-protocols.
