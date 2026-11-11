# Dynamic-Larpland

A macOS Dynamic Island / NotchNook-style widget for Wayland compositors (Hyprland first, any wlr-layer-shell
compositor supported), built with [Quickshell](https://quickshell.org).

- Collapsed: a black pill at the top of the screen showing the time.
- Hover or click: smoothly expands into media controls, a calendar, notification history; tray icons sit in the header.
- Full player with seek, a month calendar, a built-in notification daemon and an app launcher (`Super+Space`).

> Work in progress. Calendar events and cava-driven visualizer aren't there yet.

## Requirements
- Quickshell >= 0.2 (`quickshell` or `quickshell-git` on Arch/AUR)
- A compositor with wlr-layer-shell (Hyprland, Sway, niri, river…)

Stop other notification daemons (mako, dunst, swaync) — Dynamic-Larpland is the notification daemon.

## Run
```bash
/path/to/Dynamic-Larpland/scripts/run.sh
```
`run.sh` starts `qs -p <repo>` with your desktop icon theme (Quickshell only reads it from `QS_ICON_THEME`).
It detects the theme from GNOME/GTK settings or qt6ct/qt5ct; override with `DYNAMIC_LARPLAND_ICON_THEME=<name>`.
Themes are found in `~/.local/share/icons`, `~/.icons` and `/usr/share/icons`.
Autostart on Hyprland: `hl.exec_cmd("/path/to/Dynamic-Larpland/scripts/run.sh")` in
`hl.on("hyprland.start", …)` (Lua config) or `exec-once = /path/to/Dynamic-Larpland/scripts/run.sh` (hyprlang).

## Keybinds
Change them in the island's settings (gear button). Default: `Super+Space` opens the launcher.

**Hyprland (Lua config):** the settings write `~/.config/dynamic-larpland/binds.lua` and reload Hyprland.
Load it once from your config (e.g. `keybinds.lua`):
```lua
pcall(dofile, os.getenv("HOME") .. "/.config/dynamic-larpland/binds.lua")
```
Remove any other bind on the same keys (e.g. another launcher on `SUPER + Space`).

**Other compositors:** every action is available over IPC — bind these yourself:
```bash
qs -c dynamic-larpland ipc call island launcher   # also: toggle, expand, collapse, player, calendar, settings
```
e.g. Sway: `bindsym Mod4+space exec qs -c dynamic-larpland ipc call island launcher`.
