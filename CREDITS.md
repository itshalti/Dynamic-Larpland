# Credits

Dynamic-Larpland's native (C++) version is built on code from
[Noctalia](https://github.com/noctalia-dev/noctalia-shell), MIT License, Copyright (c) 2026 noctalia-dev:

- `lib/` — Noctalia's `noctalia-ui` library (renderer, scene graph, UI controls, Wayland transport), vendored
  verbatim. Its license is in `lib/LICENSE.noctalia`.
- `src/wayland/layer_shell.*`, `src/wayland/layer_surface.*` — adapted from Noctalia's `src/shell/wayland/`
  (blur support removed).
- `src/app/main_loop.*`, `src/app/*_poll_source.h` — adapted from Noctalia's `src/app/`.
- `protocols/wlr-layer-shell-unstable-v1.xml` — wlr-protocols.
