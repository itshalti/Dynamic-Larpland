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
3. **Expanded view (NotchNook-like)** — header with tabs **Nook** / **Tray** and a small **gear** button top-right.
   - *Nook*: media card (cover, title, album, artist, prev / play-pause / next, player app badge) │ week-strip
     calendar (month name, days around today, today highlighted, "Nothing for today") │ notification/message history
     with app icons.
   - *Tray*: system tray icons (StatusNotifierItem) with their menus.
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
   Stored in `~/.config/dynamic-larpland/config.json`; defaults in `config/default-config.json`.

### Visual style
Pure black (`#000`) surfaces, large radii, white primary text, grey secondary text, teal accent (`#3fd0b0`),
red-ish for weekend days. **All colors, radii, sizes, durations and easings come from `components/Theme.qml`** —
no hardcoded values in modules.

## Architecture

```
shell.qml                  entry: ShellRoot, one Island per screen (Variants over Quickshell.screens)
config/default-config.json default settings + keybinds
components/                reusable UI + Theme singleton (design tokens)
services/                  singletons (pragma Singleton), NO UI: Time, Ipc, Config, Media, Notifications,
                           Apps, Compositor
modules/island/            Island (PanelWindow + state machine), IslandShape, CollapsedView, ExpandedView
modules/media/             MediaCard, PlayerView, Visualizer
modules/calendar/          WeekStrip, MonthCalendar
modules/notifications/     NotificationList, NotificationItem
modules/tray/              TrayView
modules/launcher/          Launcher (own layer-shell window, exclusive keyboard focus), LauncherItem
modules/settings/          SettingsView, KeybindEditor
assets/icons/              bundled icons
scripts/install.sh         symlinks the repo into ~/.config/quickshell/dynamic-larpland
```

Only `shell.qml`, `components/Theme.qml`, `services/Time.qml`, `services/Ipc.qml` and `modules/island/*` exist so far;
create the others when implementing them, following this layout.

### Rules
- **services/** hold state and talk to the system; **modules/** only render and forward user intent. A module never
  runs `Process` or reads files directly.
- Import project dirs as Quickshell modules: `import qs.components`, `import qs.services`, `import qs.modules.island`.
- Island state machine (`Island.qml`, property `mode`): `collapsed | expanded | player | calendar | settings`.
  Each mode defines its target size in `Theme`; `IslandShape` animates to it. Views are `Loader`s / cross-faded items
  keyed on `mode`.
- The island window is a fixed-size transparent `PanelWindow` (layer `Overlay`, `ExclusionMode.Ignore`); input is
  limited to the pill with `mask: Region { item: shape }`, so the transparent area never steals clicks.
- Every action is reachable through IPC (`services/Ipc.qml`, target `island`): `toggle`, `expand`, `collapse`,
  `launcher`, `player`, `calendar`, `settings`. Keybinds call these; on Hyprland defaults are registered with
  `GlobalShortcut` / `hyprctl keyword bind`, elsewhere the user binds `qs -c dynamic-larpland ipc call island <action>`.

### Conventions
- English for code, comments and docs.
- Files start with `pragma ComponentBehavior: Bound` where delegates use outer ids; singletons with `pragma Singleton`.
- Use `required property` for delegate data; prefer `readonly property` for derived values.
- 4-space indent, one component per file, PascalCase file names, camelCase properties.
- Keep animations declarative (`Behavior on …` with `Theme` durations); avoid JS timers for animation.
- Don't block the UI thread: long work goes through `Process` / `FileView` asynchronously.

## Commands

```bash
qs -p .                                  # run from the repo with hot reload (save a file -> reloads)
qs -p . ipc call island toggle           # call an IPC action on the running instance
qs -p . ipc show                         # list IPC targets/functions
./scripts/install.sh && qs -c dynamic-larpland   # install as a named config and run it
```
Logs go to the terminal running `qs`; QML errors show there with file:line. There's no test suite yet — verify by
running the shell and exercising the feature (hover, click, IPC).

## Out of scope (for now)
File shelf / AirDrop, webcam "Mirror", weather, X11 support, non-wlroots compositors without layer-shell (GNOME).

## Ask the user before
Adding a non-QML dependency or native code, changing the default keybinds, replacing the notification-daemon role,
or committing / pushing.
