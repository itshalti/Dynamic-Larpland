# CLAUDE.md — Dynamic-Larpland

Dynamic-Larpland is a macOS **Dynamic Island / NotchNook**-style widget for Linux Wayland compositors.
It sits as a black pill at the top center of every screen, shows the time when idle, and smoothly expands into a
panel with media controls, a calendar, notification history, the system tray, settings and an app launcher.

## Stack (decided — don't change without asking)

- **Quickshell** (>= 0.2, QML + JavaScript on Qt 6). Pure QML, **no Rust / C++** for now. A native helper may be
  added later only if a feature truly cannot be done in QML — propose it first.
- Uses Quickshell's built-in services instead of external daemons:
  `Quickshell.Wayland` (wlr-layer-shell), `Quickshell.Services.Mpris` (players),
  `Quickshell.Services.Notifications` (notification server), `Quickshell.Services.SystemTray` (StatusNotifierItem),
  `Quickshell.Hyprland` (IPC, `GlobalShortcut`), `Quickshell.Io` (`Process`, `FileView`, `JsonAdapter`, `IpcHandler`),
  `DesktopEntries` (launcher).
- Optional runtime tools: `cava` (visualizer bars; must fall back to a fake animation without it).

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
   collapses. Width, height and corner radius animate together (spring/OutBack-style easing); content cross-fades.
   Must feel smooth — never jump sizes without a `Behavior`.
3. **Expanded view (NotchNook-like)** — header: **Nook** label, the **system tray** icons right next to it
   (StatusNotifierItem, with their menus; no separate Tray tab), and a small **gear** button top-right.
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

### Visual style
Pure black (`#000`) surfaces, large radii, white primary text, grey secondary text, teal accent (`#3fd0b0`),
red-ish for weekend days. **All colors, radii, sizes, durations and easings come from `components/Theme.qml`** —
no hardcoded values in modules.

## Architecture

```
shell.qml                  entry: ShellRoot, one Island per screen (Variants over Quickshell.screens) + Launcher
components/                Theme (design tokens singleton), Icon, IconButton, AppIcon, StyledText
services/                  singletons (pragma Singleton), NO UI:
                             Config (config.json via JsonAdapter), Time, Ipc (IpcHandler + GlobalShortcuts),
                             Compositor (Hyprland detection, focused screen, binds.lua), Media (MPRIS),
                             Notifications (daemon + history), Apps (desktop entries + fuzzy search)
modules/island/            Island (PanelWindow + mode state machine), IslandShape, ModeView, CollapsedView,
                           ExpandedView, TabButton
modules/media/             MediaCard, PlayerView, Cover, Visualizer
modules/calendar/          WeekStrip, MonthCalendar
modules/notifications/     NotificationList, NotificationItem
modules/tray/              TrayView
modules/launcher/          Launcher (own full-screen layer-shell window, exclusive keyboard), LauncherItem
modules/settings/          SettingsView, KeybindEditor, Toggle
assets/icons/              bundled white SVG icons (own drawings), loaded with components/Icon
scripts/install.sh         symlinks the repo into ~/.config/quickshell/dynamic-larpland
```

Runtime files: `~/.config/dynamic-larpland/config.json`, `~/.config/dynamic-larpland/binds.lua` (generated),
`~/.local/state/dynamic-larpland/notifications.json` (history).

### Rules
- **services/** hold state and talk to the system; **modules/** only render and forward user intent. A module never
  runs `Process` or reads files directly.
- Import project dirs as Quickshell modules: `import qs.components`, `import qs.services`, `import qs.modules.island`.
- Island state machine (`Island.qml`, property `mode`): `collapsed | expanded | player | calendar | settings`.
  Each mode defines its target size in `Theme`; `IslandShape` animates to it. Each page is wrapped in a `ModeView`
  that lays it out at its final size (no reflow during the morph) and cross-fades it on `mode`.
- Hover expands after `hoverDelayMs`, leaving collapses after `Theme.hoverCloseDelay` unless `holdOpen` (settings
  page, open tray menu). Keybind/IPC actions only affect the island on the focused screen (`Island.isTarget`).
- The island window is a fixed-size transparent `PanelWindow` (layer `Overlay`, `ExclusionMode.Ignore`); input is
  limited to the pill with `mask: Region { item: shape }`, so the transparent area never steals clicks.
- Every action is reachable through IPC (`services/Ipc.qml`, target `island`): `toggle`, `expand`, `collapse`,
  `launcher`, `player`, `calendar`, `settings`.
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
- The session also starts `dev/wallpaper` (gradient background, separate Quickshell config) and
  `dev/fake-player.py` ("Demo Player": made-up tracks with generated covers on the session bus; control it with
  `dbus-send … org.mpris.MediaPlayer2.Player.Next` etc.).
- README media live in `docs/media/` and are recorded in the nested session: `dev/record.py OUT.gif X Y W H SECONDS
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
