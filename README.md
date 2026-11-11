# Dynamic-Larpland

**A macOS-style Dynamic Island for Linux** — a black pill at the top of your screen that shows the time and
smoothly opens into your music, calendar and notifications. Made for Hyprland, works on any Wayland compositor
with layer-shell (Sway, niri, river…). Built with [Quickshell](https://quickshell.org).

<p align="center">
  <img src="docs/media/tour.gif" alt="Tour: notification, hover to expand, player, week carousel, month calendar, settings" width="860">
</p>

## Features

- 🕒 **Clock pill** — sits quietly at the top; shows the album cover and live bars while music plays.
- ✨ **Smooth expand** — hover or click it and it morphs open; move away and it closes.
- 🎵 **Music** — controls for Spotify, browsers, mpv and any other player. Click the card for the full player with
  a seek bar.
- 📅 **Calendar** — scroll the week strip like a carousel; click it for the month view and click any day.
- 🔔 **Notifications** — new ones pop out of the pill, then stay in a history with app icons.
- 🧩 **System tray** — your tray icons live in the island's header.
- 🚀 **App launcher** — `Super + Space`, fuzzy search, icons from your icon theme.
- ⚙️ **Settings** — change keybinds right in the island (gear button).

<p align="center">
  <img src="docs/media/launcher.gif" alt="The app launcher opening and closing" width="640">
</p>

## Install

1. Install [Quickshell](https://quickshell.org) 0.2 or newer (Arch: `pacman -S quickshell`).
2. Get the code:
   ```bash
   git clone https://github.com/itshalti/Dynamic-Larpland.git ~/Dynamic-Larpland
   ```
3. Turn off your current notification daemon (mako, dunst, swaync, or your shell's notifications) —
   Dynamic-Larpland takes over that job.
4. Start it:
   ```bash
   ~/Dynamic-Larpland/scripts/run.sh
   ```

**Start it with Hyprland** — add to your config:

| Config type | Line |
| --- | --- |
| Lua (`hyprland.lua`, Hyprland 0.55+) | `hl.on("hyprland.start", function() hl.exec_cmd(os.getenv("HOME") .. "/Dynamic-Larpland/scripts/run.sh") end)` |
| Classic (`hyprland.conf`) | `exec-once = ~/Dynamic-Larpland/scripts/run.sh` |

## Using it

| Do this | To get |
| --- | --- |
| Hover or click the pill | Open the island |
| Move the mouse away (or click it, then `Esc`) | Close it |
| Click the music card | Full player (click the cover to go back) |
| Scroll over the week strip | Spin the days |
| Click the week strip | Month calendar |
| Right-click a tray icon | Its menu |
| `Super + Space` | App launcher (`↑` `↓` `Enter`, `Esc` to close) |

## Keybinds

Open the island → gear button → click a keybind and press the new keys
(`Esc` cancels, `Backspace` removes it).

**Hyprland:** add this line once to your config (e.g. `keybinds.lua`) so the keybinds work:

```lua
pcall(dofile, os.getenv("HOME") .. "/.config/dynamic-larpland/binds.lua")
```

If something else already uses `Super + Space` (another launcher), remove that bind.

**Other compositors:** bind keys to these commands yourself, e.g. in Sway:

```
bindsym Mod4+space exec qs -p ~/Dynamic-Larpland ipc call island launcher
```

Available actions: `launcher`, `toggle`, `expand`, `collapse`, `player`, `calendar`, `settings`.

## Troubleshooting

- **No icons in the launcher** — start it with `scripts/run.sh`, not plain `qs`. To pick a theme by hand:
  `DYNAMIC_LARPLAND_ICON_THEME=Papirus ~/Dynamic-Larpland/scripts/run.sh`.
- **Notifications don't show up** — another notification daemon is still running; stop it.
- **Settings and history** live in `~/.config/dynamic-larpland/` and `~/.local/state/dynamic-larpland/`.

## Contributing

`./scripts/dev.sh` opens a nested Hyprland window with the island inside, so you can hack on it without touching
your desktop. See [`CLAUDE.md`](CLAUDE.md) for the project layout and conventions.
