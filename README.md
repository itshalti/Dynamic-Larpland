# Dynamic-Larpland

A macOS Dynamic Island / NotchNook-style widget for Wayland compositors (Hyprland first, any wlr-layer-shell
compositor supported), built with [Quickshell](https://quickshell.org).

- Collapsed: a black pill at the top of the screen showing the time.
- Hover or click: smoothly expands into media controls, a calendar, notification history and the system tray.
- Full player with seek, a month calendar, a built-in notification daemon and an app launcher (`Super+Space`).

> Work in progress. Calendar events and cava-driven visualizer aren't there yet.

## Requirements
- Quickshell >= 0.2 (`quickshell` or `quickshell-git` on Arch/AUR)
- A compositor with wlr-layer-shell (Hyprland, Sway, niri, river…)

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
Autostart on Hyprland: `hl.exec_cmd("qs -c dynamic-larpland")` in `hl.on("hyprland.start", …)` (Lua config)
or `exec-once = qs -c dynamic-larpland` (hyprlang).

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
