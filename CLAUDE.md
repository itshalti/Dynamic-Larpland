# CLAUDE.md — Dynamic-Larpland

Dynamic-Larpland is a macOS **Dynamic Island / NotchNook**-style widget for Linux Wayland compositors.
It sits as a black pill at the top center of every screen, shows the time when idle, and smoothly expands into a
panel with media controls, a calendar, notification history, the system tray, settings and an app launcher.

## Native rewrite in progress (branch `cpp-rewrite`)

The user decided to rewrite the island in **C++ on Noctalia's `noctalia-ui` library** (MIT, vendored verbatim in
`lib/` — never edit it; attribution in `CREDITS.md`). Plan: `~/.claude/plans/logical-enchanting-minsky.md`
(milestones 1–7; the QML tree below stays until milestone 7). On this branch:
- Build: `meson setup build && ninja -C build` (deps: meson, ninja, stb + the libs Noctalia uses).
- `src/` = native code: `app/` (main loop, bootstrap), `config/` (TOML at `~/.config/dynamic-larpland/config.toml`,
  inotify reload), `ipc/` (socket `$XDG_RUNTIME_DIR/dynamic-larpland-$WAYLAND_DISPLAY.sock`), `island/` (layer
  surface per output, modes, `theme.h` tokens), `wayland/` (layer shell, from Noctalia).
- `scripts/dev.sh` builds and runs the native binary in nested (log: `.dev/island.log`); drive it with
  `WAYLAND_DISPLAY=wayland-2 build/dynamic-larpland msg <action>`; ALT+I toggles.
- Demo data on the nested bus: `dev/fake-player.py` (MPRIS) and `dev/fake-tray.py` (tray item with a DBusMenu),
  started by hand with `hyprctl -i 1 dispatch "hl.dsp.exec_cmd(\"python3 $PWD/dev/fake-tray.py\")"`.
- Settings window (`src/settings`): a centered layer surface (not a toplevel, so it's centered on any
  compositor) built from noctalia-ui `Flex`/`ScrollView` + controls (Select needs the window's
  `SelectDropdownPopup` set as popup context). Changes save to config.toml immediately. Keybinds are recorded by
  the window itself (KeybindRecorder rejects Super) and `compositors/keybinds.cpp` writes Hyprland's binds.lua
  (`hl.dsp.exec_cmd("<binary> msg <action>")`).
- Lock screen (`src/lock`, ext-session-lock-v1 + PAM "login" via a re-exec'd `pam-helper`): test it only in
  nested; you can't unlock from the tools (no key injection), so restart the nested session afterwards.
  Power actions (`system:*`) are page-only, never IPC actions.
- Pages hold raw node pointers in animations: `Islands::rebuildPage` cancels the whole subtree's animations
  before destroying it — keep that when adding pages. Lambdas stored in nodes must never capture the
  `PageContext` (it's a temporary); capture the services/callbacks they need.

## Stack (decided — don't change without asking)

- **Quickshell** (>= 0.2, QML + JavaScript on Qt 6). Pure QML, **no Rust / C++** for now. A native helper may be
  added later only if a feature truly cannot be done in QML — propose it first.
- Uses Quickshell's built-in services instead of external daemons:
  `Quickshell.Wayland` (wlr-layer-shell), `Quickshell.Services.Mpris` (players),
  `Quickshell.Services.Notifications` (notification server), `Quickshell.Services.SystemTray` (StatusNotifierItem),
  `Quickshell.Hyprland` (IPC, `GlobalShortcut`), `Quickshell.Io` (`Process`, `FileView`, `JsonAdapter`, `IpcHandler`),
  `DesktopEntries` (launcher).
- Optional runtime tools: `cava` (visualizer bars; must fall back to a fake animation without it),
  `gpu-screen-recorder` (record button), `matugen` (wallpaper colors). Without them only that feature fails.

## Compositor support

- **Hyprland is the primary target.** Hyprland-only features (global shortcuts, `hyprctl` binds, focused monitor)
  go through `services/Compositor.qml`.
- The core must work on **any wlr-layer-shell compositor** (Sway, niri, river…). Guard every Hyprland-specific call
  with `Compositor.isHyprland` and degrade gracefully (e.g. show the bind line for the user to paste instead of
  applying it).

## Features (target spec)

1. **Collapsed island** — black pill, top center, shows only the time `HH:mm`. May briefly show compact "live
   activities": playing track (tiny cover + equalizer bars), a new notification (app icon + title).
2. **Expand animation** — hover (short delay) or click expands; leaving the pointer / clicking outside / `Esc`
   collapses. Width, height and corner radius animate together (`Theme.easingMorph`, OutQuint — no bounce); content cross-fades.
   Must feel smooth — never jump sizes without a `Behavior`.
3. **Expanded view (NotchNook-like)** — header: **Nook** label, the **system tray** icons right next to it
   (StatusNotifierItem, with their menus; no separate Tray tab), and top-right: **record**, **wallpapers**, **gear**, **power**.
   - Body: media card (cover, title, album, artist, prev / play-pause / next, player app badge) │ week-strip
     calendar (month name, days around today, today highlighted, "Nothing for today") │ notification/message history
     with app icons.
4. **Player view** — left-click on the media card morphs the island into the full player: cover, title/artist,
   progress bar with elapsed / remaining time and click/drag seek, prev / play-pause / next, visualizer. Supports
   multiple MPRIS players with switching.
5. **Big calendar** — click on the week strip → month grid (Sun-first by default), prev/next month arrows, today in an
   accent circle, other-month days dimmed.
6. **Notifications** — the island **is the notification daemon** (replaces mako/dunst). It shows new notifications as
   a live activity and keeps a persisted history (`~/.local/state/dynamic-larpland/notifications.json`) with icons,
   dismiss and clear-all.
7. **Launcher** — default `Super+Space`. Rounded black window: search field, fuzzy-filtered `.desktop` apps (icon,
   name, generic name), ↑/↓ navigation, Enter launches, Esc closes; selected row has a highlighted background and an
   accent bar on the left.
8. **Settings** (gear) — keybind editor (launcher, toggle island, open player, open calendar) + basic options.
   Stored in `~/.config/dynamic-larpland/config.json` (created on first run); the defaults live in the
   `JsonAdapter` inside `services/Config.qml`.
9. **Screen recording** — the record button toggles `gpu-screen-recorder` on the focused monitor (desktop audio,
   60 fps) into `Config.recorder.dir` (`~/Videos`); the button turns red with a stop icon while recording.
10. **Wallpapers** — the image button opens a strip of previews from `Config.wallpaper.dir` (`~/Pictures`, editable
   in settings). Picking one: the island draws it (own `Background` layer window per screen, cross-fade) and runs
   `matugen` with `assets/matugen/*` templates → island accent (`colors.json` in the state dir), GTK 3/4
   (`gtk-*/dynamic-larpland.css`), kitty (`kitty/themes/dynamic-larpland.conf`), Hyprland borders (`colors.lua`,
   loaded from `binds.lua`); then `scripts/apply-colors.sh` adds the include lines once and reloads.
11. **Power** — the power button opens a page: shut down, reboot, reboot to UEFI (`systemctl`; these three need a
   second click within `Theme.confirmTimeout`), lock (the island's own lock screen), sleep (`systemctl suspend`).
12. **Lock screen** — `WlSessionLock` (`modules/lock`) with a surface per screen: current wallpaper (dimmed), big clock
   + date in the center; typing reveals a pill field at the bottom showing one `*` per character; Enter checks the
   password through PAM (`services/Lock`, config `passwd`), a wrong one shakes the field. Don't hot-reload while
   locked in dev (the old surface stays up); restart the nested session instead.

### Visual style
Pure black (`#000`) surfaces, large radii, white primary text, grey secondary text, teal accent (`#3fd0b0`),
red-ish for weekend days. **All colors, radii, sizes, durations and easings come from `components/Theme.qml`** —
no hardcoded values in modules.

## Architecture

```
shell.qml                  entry: ShellRoot, one Background + one Island per screen (Variants) + Launcher
components/                Theme (design tokens singleton), Icon, IconButton, AppIcon, StyledText
services/                  singletons (pragma Singleton), NO UI:
                             Config (config.json via JsonAdapter), Time, Ipc (IpcHandler + GlobalShortcuts),
                             Compositor (Hyprland detection, focused screen, binds.lua), Media (MPRIS),
                             Notifications (daemon + history), Apps (desktop entries + fuzzy search),
                             Recorder (gpu-screen-recorder), Wallpaper (folder, current, matugen colors),
                             Power (systemctl actions), Lock (session lock state + PAM)
modules/island/            Island (PanelWindow + mode state machine), IslandShape, ModeView, CollapsedView,
                           ExpandedView, TabButton
modules/media/             MediaCard, PlayerView, Cover, Visualizer
modules/calendar/          WeekStrip, MonthCalendar
modules/notifications/     NotificationList, NotificationItem
modules/tray/              TrayView
modules/launcher/          Launcher (own full-screen layer-shell window, exclusive keyboard), LauncherItem
modules/settings/          SettingsView, KeybindEditor, Toggle
modules/power/             PowerView, PowerButton
modules/lock/              LockScreen (WlSessionLock), LockSurface
modules/wallpaper/         Background (wallpaper window), WallpaperView (picker), WallpaperItem
assets/matugen/            matugen templates (island.json, gtk.css, kitty.conf, hyprland.lua)
assets/icons/              bundled white SVG icons (own drawings), loaded with components/Icon
scripts/apply-colors.sh    after matugen: add GTK/kitty include lines once, reload kitty + Hyprland
scripts/install.sh         symlinks the repo into ~/.config/quickshell/dynamic-larpland
```

Runtime files: `~/.config/dynamic-larpland/config.json`, `~/.config/dynamic-larpland/binds.lua` (generated),
`~/.local/state/dynamic-larpland/notifications.json` (history), `matugen.toml` + `colors.lua` (config dir) and
`colors.json` (state dir) generated on wallpaper pick.

### Rules
- **services/** hold state and talk to the system; **modules/** only render and forward user intent. A module never
  runs `Process` or reads files directly.
- Import project dirs as Quickshell modules: `import qs.components`, `import qs.services`, `import qs.modules.island`.
- Island state machine (`Island.qml`, property `mode`): `collapsed | expanded | player | calendar | settings | wallpapers | power`.
  Each mode defines its target size in `Theme`; `IslandShape` animates to it. Each page is wrapped in a `ModeView`
  that lays it out at its final size (no reflow during the morph) and cross-fades it on `mode`.
- Hover expands after `hoverDelayMs`, leaving collapses after `Theme.hoverCloseDelay` (`modeSwitchCloseDelay` if the pointer
  left within `modeSwitchGrace` of a page switch — the pill resizes under it) unless `holdOpen` (settings
  page, open tray menu). Keybind/IPC actions only affect the island on the focused screen (`Island.isTarget`).
- The island window is a fixed-size transparent `PanelWindow` (layer `Overlay`, `ExclusionMode.Ignore`); input is
  limited to the pill with `mask: Region { item: shape }`, so the transparent area never steals clicks.
- Every action is reachable through IPC (`services/Ipc.qml`, target `island`): `toggle`, `expand`, `collapse`,
  `launcher`, `player`, `calendar`, `settings`, `wallpapers`, `record`, `power`, `lock`.
- Keybinds on Hyprland (0.55+ Lua config): actions are registered as global shortcuts `dynamic-larpland:<action>`;
  `Compositor.writeBinds()` generates `binds.lua` (`hl.bind(keys, hl.dsp.global(...))`) from the config and runs
  `hyprctl reload config-only`. The user's Hyprland config must contain
  `pcall(dofile, "<config dir>/dynamic-larpland/binds.lua")`. Elsewhere the user binds
  `qs -p <repo> ipc call island <action>` in their compositor.

- Icons from the system theme (`Quickshell.iconPath`, `IconImage`, `hasThemeIcon`) need `QS_ICON_THEME`;
  `scripts/run.sh` sets it from the desktop settings (dev.sh passes it through).

### Conventions
- English for code, comments and docs.
- No inline `component X: …` declarations — Quickshell's `qs.*` module loader rejects them ("X is not a type");
  put each component in its own file.
- Never name a property `onSomething` (QML treats it as a signal handler).
- Files start with `pragma ComponentBehavior: Bound` where delegates use outer ids; singletons with `pragma Singleton`.
- Use `required property` for delegate data; prefer `readonly property` for derived values.
- 4-space indent, one component per file, PascalCase file names, camelCase properties.
- Keep animations declarative (`Behavior on …` with `Theme` durations); avoid JS timers for animation.
- Don't block the UI thread: long work goes through `Process` / `FileView` asynchronously.

## Development: always in a nested Hyprland

**Never run the island on the user's real desktop and never touch their Hyprland config or their shell
(Noctalia) while developing.** It would fight the host shell for the notification service and keybinds.
Develop inside a nested Hyprland window instead:

```bash
./scripts/dev.sh               # nested Hyprland window (own D-Bus session) running scripts/run.sh
./scripts/dev.sh --shared-bus  # same, but on the host D-Bus to see host media players
```
- Config: `dev/hyprland-nested.lua`. Modifier inside is **ALT** (the host grabs SUPER): ALT+Space launcher,
  ALT+I toggle, ALT+Return kitty, ALT+N test notification, ALT+Q close, ALT+SHIFT+E quit.
- The island there uses `XDG_CONFIG_HOME=.dev/config`, `XDG_STATE_HOME=.dev/state` (gitignored); `dev.sh` seeds
  ALT keybinds. The real `~/.config/dynamic-larpland` is not used.
- Start it in a terminal tab so the user can see it; stop it (Ctrl-C) when done. Hot reload works as usual.
- Talking to it from the host: `qs -p . ipc …` does NOT find the nested instance — use
  `qs ipc --pid $(pgrep -f '^qs -p') call island <action>`; `qs log --pid <qs pid>` for logs; `hyprctl -i 1 …` targets the nested Hyprland (`hyprctl instances` lists them); the nested window is
  the host client with class `aquamarine` (use its `at`/`size` for `grim -g` screenshots). For a test notification
  on the nested bus: `DBUS_SESSION_BUS_ADDRESS=$(tr '\0' '\n' </proc/<qs pid>/environ | sed -n
  's/^DBUS_SESSION_BUS_ADDRESS=//p') notify-send …`.
- The session also starts `dev/wallpaper` (gradient background, separate Quickshell config).
  `dev/fake-player.py` ("Demo Player": made-up tracks with generated covers) is not autostarted; run it on the
  nested bus by hand when a player is needed (e.g. for recordings).
- README media (keep it to `docs/media/tour.gif` — one tour of the island — and `launcher.gif`) are recorded in the
  nested session: `dev/record.py OUT.gif X Y W H SECONDS
  [--cursor] [--width 840]` (grim frames → ffmpeg GIF); stills with `grim -g`. Drive the island with IPC while
  recording; only demo data on screen (fake player, made-up notifications), no personal paths.
- Hot reload is unreliable (it skips edits after a while). When an edit doesn't show up, restart only the island
  inside the nested session: `kill $(pgrep -f '^qs -p')` then
  `hyprctl -i 1 dispatch "hl.dsp.exec_cmd(\"env XDG_CONFIG_HOME=$PWD/.dev/config XDG_STATE_HOME=$PWD/.dev/state $PWD/scripts/run.sh\")"`.

## Commands

```bash
./scripts/run.sh                         # run with the desktop icon theme + hot reload (what dev.sh starts inside)
qs -p . ipc call island toggle           # call an IPC action on the running instance
qs -p . ipc show                         # list IPC targets/functions
```
QML errors show in the log with file:line. There's no test suite yet — verify by running the nested session and
exercising the feature (hover, click, IPC, screenshots).

## Out of scope (for now)
File shelf / AirDrop, webcam "Mirror", weather, X11 support, non-wlroots compositors without layer-shell (GNOME).

## Ask the user before
Adding a non-QML dependency or native code, changing the default keybinds, replacing the notification-daemon role,
or committing / pushing.
