# Dynamic-Larpland

A macOS Dynamic Island / NotchNook-style widget for Wayland compositors (Hyprland first, any wlr-layer-shell
compositor supported), built with [Quickshell](https://quickshell.org).

- Collapsed: a black pill at the top of the screen showing the time.
- Hover or click: smoothly expands into media controls, a calendar, notification history and the system tray.
- Full player with seek, a month calendar, a built-in notification daemon and an app launcher (`Super+Space`).

> Work in progress — right now only the collapsed clock and the expand animation exist.

## Requirements
- Quickshell >= 0.2 (`quickshell` or `quickshell-git` on Arch/AUR)
- A compositor with wlr-layer-shell (Hyprland, Sway, niri, river…)
- Optional: `cava` for the music visualizer

Stop other notification daemons (mako, dunst, swaync) — Dynamic-Larpland is the notification daemon.

## Run
```bash
qs -p /path/to/Dynamic-Larpland
```
Or install it as a named config:
```bash
./scripts/install.sh
qs -c dynamic-larpland
```
Autostart on Hyprland: `exec-once = qs -c dynamic-larpland`.

## Keybinds
All actions are available over IPC:
```bash
qs -c dynamic-larpland ipc call island toggle   # also: expand, collapse, launcher, player, calendar, settings
```
Default: `Super+Space` opens the launcher. On other compositors bind the IPC command yourself, e.g. in Sway:
`bindsym Mod4+space exec qs -c dynamic-larpland ipc call island launcher`.
