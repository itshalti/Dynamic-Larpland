# CLAUDE.md — Dynamic-Larpland

Dynamic-Larpland is a macOS **Dynamic Island / NotchNook**-style widget for Linux Wayland compositors.
It sits as a black pill at the top center of every screen, shows the time when idle, and smoothly expands into a
panel with media controls, a calendar, notification history, the system tray, wallpapers, power and an app launcher.

## Stack (decided — don't change without asking)

- **Native C++23**, built with **meson**. The UI/render/Wayland layer is Noctalia's **noctalia-ui** library (MIT),
  vendored **verbatim** in `lib/` — never edit it; attribution in `CREDITS.md`, license in `lib/LICENSE.noctalia`.
- Ported from Noctalia (adapted, see `CREDITS.md`): theme engine (`src/theme`), MPRIS, tray, session bus, HTTP client,
  icon resolver, desktop entries, PAM authenticator, layer-shell surface, main loop. Own code: island, pages,
  notification daemon, launcher, lock screen, settings window, config, IPC, recorder, keybinds.
- Vendored third party: `third_party/material_color_utilities` (Apache-2.0), `third_party/fzy` (MIT).
- System libs: wayland, EGL/GLES (libglvnd), xkbcommon, cairo, pango, harfbuzz, fontconfig, librsvg, libwebp, libjxl,
  sdbus-c++, toml++, nlohmann-json, curl, pam, stb (build). Optional at runtime: `gpu-screen-recorder`.
- No runtime dependency on Noctalia, Quickshell or matugen.

## Compositor support

- **Hyprland is the primary target**: focused monitor via its IPC socket (`src/compositors/hyprland.*`), keybinds via
  a generated `binds.lua` (`src/compositors/keybinds.*`).
- Everything else needs only wlr-layer-shell (+ ext-session-lock for the lock screen). Without Hyprland, actions go to
  every island and the settings page shows the `dynamic-larpland msg <action>` command to bind.

## Features

1. **Collapsed pill** — clock (`island.time_format`); live activities: playing track (cover, clock, bars) and a new
   notification (icon, title, app) for `notifications.popup_timeout_ms`.
2. **Morph** — hover (after `island.hover_delay_ms`) or click opens; leaving collapses after 0.3 s (1.5 s right after
   a page switch, the pill resizes under the pointer); Esc collapses. Width, height and radius animate together
   (EaseOutCubic, no bounce); pages cross-fade.
3. **Expanded page** — header: Nook tab, tray icons (menus as popups), record / wallpapers / settings / power buttons.
   Body: media card │ week strip carousel │ notification history.
4. **Player** — cover, title/artist, progress with click/drag seek, prev/play/next.
5. **Month calendar** — prev/next month, today in an accent circle, selectable days.
6. **Notifications** — the island is the daemon (`org.freedesktop.Notifications`), history in
   `~/.local/state/dynamic-larpland/notifications.json`, icons from the theme or `image-data`.
7. **Launcher** — own overlay surface, fuzzy search (fzy), icons, keyboard and pointer navigation.
8. **Wallpapers** — picker page (folder `wallpaper.dir`), the island draws the wallpaper (Background layer,
   cross-fade); a pick runs `dynamic-larpland theme <img> -o palette.json --apply <ids>` in a child process: palette
   for the island accent + the enabled built-in app templates (`assets/templates`, outputs `dynamic-larpland.*`).
9. **Power** — shut down / reboot / UEFI need a second click; these are page-only (`system:*`), never IPC.
10. **Lock screen** — ext-session-lock-v1, wallpaper + clock, masked field, PAM `login` via a re-exec'd `pam-helper`.
11. **Recorder** — gpu-screen-recorder on the focused monitor into `recorder.dir`, SIGINT to stop.
12. **Settings window** — centered layer surface (not a toplevel, so it's centered everywhere) with Look & theme,
    Island & keybinds, Notifications & calendar, Recorder/lock/power. Saves to `config.toml` immediately.

### Visual style
Pure black surfaces, large radii, white primary / grey secondary text, accent from the wallpaper palette (teal
`#3fd0b0` until one exists), red-ish weekends. **All tokens live in `src/island/theme.h`** — no hardcoded values in
pages. noctalia-ui's own palette (menus, controls) is set from them in `src/island/ui_palette.cpp`.

## Layout

```
meson.build            build; assets installed to <datadir>/dynamic-larpland/assets
lib/                   noctalia-ui (verbatim)          third_party/  MCU, fzy
src/main.cpp           CLI: run | msg <action> | theme <image> | pam-helper (internal)
src/app/               Application (owns everything, routes input), MainLoop, library bootstrap
src/config/            ConfigService: config.toml (toml++), inotify reload; atomic_file, toml.h
src/ipc/               unix socket $XDG_RUNTIME_DIR/dynamic-larpland-$WAYLAND_DISPLAY.sock
src/compositors/       Hyprland focused monitor, keybinds -> binds.lua
src/island/            Islands (surface per output, modes, morph), theme.h, widgets, pages:
                       collapsed / expanded / media / calendar / notification_list / tray_view / wallpaper / power
src/services/          media (MPRIS + art), notifications (daemon), wallpaper (+palette), recorder
src/dbus/ src/net/     session bus, MPRIS, tray, HTTP (ported)
src/theme/             palette generation + template engine (ported), theme_cli
src/launcher/ src/lock/ src/settings/ src/wallpaper/ src/wayland/ src/system/ src/auth/
assets/templates/      app templates (from Noctalia, renamed outputs)
dev/                   nested session config, demo player / tray / wallpapers, GIF recorder
scripts/               install.sh (release build into ~/.local), dev.sh (nested session)
```

## Rules

- **Pages only render and forward intent**; services own state and talk to the system.
- Pages are rebuilt from state (`Islands::rebuildPage`). It cancels the old subtree's animations first — animations
  hold raw node pointers. Lambdas stored in nodes must never capture the `PageContext` (a temporary); capture the
  services/callbacks they need.
- Anything that may rebuild the page that triggered it goes through `ctx.action(...)` (deferred).
- Worker-thread results (process exit, PAM) come back via `DeferredCall::callLater`.
- New layer surfaces: copy the pattern of `Launcher` / `SettingsWindow` (prepare-frame builds the scene, Flex trees
  need `root->layout(renderer)`, `Select` needs a `SelectDropdownPopup` popup context).
- Every action is reachable through IPC except the power ones.

### Conventions
- English for code, comments and docs. 2-space indent, the same style as Noctalia's code.
- Never edit `lib/`. If it lacks something, add it in `src/`.
- No warnings in `src/` (`ninja -C build` must be clean).

## Development: always in a nested Hyprland

**Never run the island on the user's real desktop and never touch their Hyprland config or their shell
(Noctalia).** It would fight the host shell for the notification service and keybinds.

```bash
./scripts/dev.sh               # builds, then a nested Hyprland window (own D-Bus) running build/dynamic-larpland
./scripts/dev.sh --shared-bus  # same, on the host D-Bus (see host players; host keeps notifications)
```
- Config `dev/hyprland-nested.lua`; modifier **ALT**: ALT+I toggle, ALT+Space launcher, ALT+Return kitty,
  ALT+N test notification, ALT+SHIFT+E quit. Island config/state in `.dev/` (gitignored), seeded with demo
  wallpapers (`dev/make-wallpapers.py`) and `.dev/videos` for recordings. Log: `.dev/island.log`.
- Drive it from the host: `WAYLAND_DISPLAY=wayland-2 build/dynamic-larpland msg <action>`; screenshots with
  `WAYLAND_DISPLAY=wayland-2 grim …`; `hyprctl -i 1 …` targets the nested Hyprland. Restart only the island:
  `msg quit`, then `hyprctl -i 1 dispatch "hl.dsp.exec_cmd(\"env XDG_CONFIG_HOME=$PWD/.dev/config XDG_STATE_HOME=$PWD/.dev/state $PWD/build/dynamic-larpland > $PWD/.dev/island.log 2>&1\")"`.
- Demo data on the nested bus: `dev/fake-player.py` (MPRIS), `dev/fake-tray.py` (tray item with a DBusMenu),
  `notify-send` with the nested `DBUS_SESSION_BUS_ADDRESS` (from `/proc/<pid>/environ`).
- The lock screen can't be unlocked from the tools: don't quit the island while nested is locked (Hyprland shows
  "lockscreen died"; clear it with `hyprctl -i 1 eval 'hl.clear_crashed_lockscreen()'`).
- Don't test templates that touch the host (GTK's `gsettings`, Hyprland reload) outside `.dev`.
- README media (`docs/media/tour.gif`, `launcher.gif`) are recorded in nested with `dev/record.py`; only demo data.

## Commands

```bash
meson setup build && ninja -C build      # debug build
./scripts/install.sh                     # release build + install into ~/.local
build/dynamic-larpland msg toggle        # IPC (any action)
build/dynamic-larpland theme IMG --dark  # palette JSON
```
No test suite yet — verify in the nested session (IPC, screenshots, the log).

## Out of scope (for now)
File shelf / AirDrop, webcam "Mirror", weather, X11, compositors without layer-shell (GNOME).

## Ask the user before
Adding a dependency, changing default keybinds, replacing the notification-daemon role, committing / pushing,
merging `cpp-rewrite` into `main`.
