# Dynamic-Larpland

A macOS-style **Dynamic Island** for Linux Wayland. A small black pill sits at the top of every screen and shows the
time. Hover or click it and it smoothly opens into a panel with your music, calendar, notifications and tray.
Made for **Hyprland**, and it works on any compositor with layer-shell (Sway, niri, river…).

Native C++ built on [Noctalia](https://github.com/noctalia-dev/noctalia-shell)'s UI library (see [Credits](#credits)).
Nothing else needs to be running: no Quickshell, no Noctalia, no matugen.

<p align="center">
  <img src="docs/media/tour.gif" alt="The island opening, the player, calendar, wallpapers and settings" width="840">
</p>

## Features

- 🕒 **Clock pill.** It stays small at the top of the screen. While music plays it shows the album cover and live bars, and new
  notifications pop out of it for a moment.
- ✨ **Smooth expand.** Hover or click it and it morphs open; move away and it closes.
- 🎵 **Music.** Controls for Spotify, browsers, mpv and any other MPRIS player. Click the card to open the full player with
  a seek bar.
- 📅 **Calendar.** Scroll the week strip like a carousel, or click it for the month view, where any day can be selected.
- 🔔 **Notifications.** The island is your notification daemon, with a history that keeps app icons.
- 🧩 **System tray.** Tray icons and their menus live in the island's header.
- 🚀 **App launcher.** `Super + Space`, fuzzy search, icons from your icon theme.
- 🖼️ **Wallpapers and colors.** Pick a wallpaper from your folder and the island draws it. A Material You palette is made
  from it, and your apps (GTK, Qt, kitty, Hyprland borders, btop, cava and 14 more) are recolored to match.
- ⏺️ **Screen recording.** One button records the screen to `~/Videos` (needs `gpu-screen-recorder`).
- ⏻ **Power and lock.** Shut down, reboot, reboot to UEFI, sleep, and a lock screen with your wallpaper and a big clock.
- ⚙️ **Settings.** A settings window in the island's style, like Noctalia's: search, Advanced/Overridden
  filters, reset. Theme and day/night switching by your location, built-in and community app templates, keybinds,
  notifications (Do not disturb, per-app mute/block), idle (lock, screens off, suspend), lock screen (own
  wallpaper, fingerprint), recorder.

<p align="center">
  <img src="docs/media/launcher.gif" alt="The app launcher" width="640">
</p>

## Install

1. Install the build tools and libraries. On Arch:
   ```bash
   sudo pacman -S --needed base-devel meson ninja stb wayland wayland-protocols libglvnd libxkbcommon \
     cairo pango harfbuzz fontconfig librsvg libwebp libjxl sdbus-cpp tomlplusplus nlohmann-json curl pam
   ```
   Optional: `gpu-screen-recorder` for the record button.
2. Get the code and build it. This installs into `~/.local` and needs no root:
   ```bash
   git clone https://github.com/itshalti/Dynamic-Larpland.git ~/Dynamic-Larpland
   ~/Dynamic-Larpland/scripts/install.sh
   ```
3. Turn off your current notification daemon (mako, dunst, swaync, or your shell's notifications), because
   Dynamic-Larpland takes over that job. Then start it with `dynamic-larpland`.

**Start it with Hyprland.** Add to `hyprland.lua`:

```lua
hl.on("hyprland.start", function() hl.exec_cmd(os.getenv("HOME") .. "/.local/bin/dynamic-larpland") end)
-- keybinds set in the settings window
pcall(dofile, os.getenv("HOME") .. "/.config/dynamic-larpland/binds.lua")
```

On a classic `hyprland.conf`, or on another compositor, start `~/.local/bin/dynamic-larpland` from its autostart
and bind keys to the commands below.

## Using it

| Do this | To get |
| --- | --- |
| Hover or click the pill | Open the island |
| Move the mouse away, or press `Esc` | Close it |
| Click the music card | Full player (click the cover to go back) |
| Scroll over the week strip | Spin the days |
| Click the week strip | Month calendar |
| Click / right-click a tray icon | Activate it / its menu |
| Header buttons ⏺ 🖼 ⚙ ⏻ | Record, wallpapers, settings, power |
| `Super + Space` | App launcher (`↑` `↓` `Enter`, `Esc` to close) |

## Keybinds and commands

Set keybinds in **Settings → Island & keybinds**: click a key and press the new combo. On Hyprland they go into
`~/.config/dynamic-larpland/binds.lua`, which the line above loads. Every action is also a command you can bind
anywhere:

```bash
dynamic-larpland msg toggle      # also: expand, collapse, player, calendar, wallpapers, power,
                                 #       launcher, settings, record, lock, dnd
dynamic-larpland msg settings notifications   # open a settings section
dynamic-larpland msg wallpaper ~/Pictures/forest.jpg
```

## Colors from your wallpaper

Picking a wallpaper makes a palette from it and recolors the apps you ticked in **Settings → Look & theme**. You
can also do this from a terminal:

```bash
dynamic-larpland theme ~/Pictures/forest.jpg --scheme m3-content          # palette as JSON
dynamic-larpland theme ~/Pictures/forest.jpg --apply gtk3,gtk4,kitty      # recolor apps
```

Recolored apps load a `dynamic-larpland.*` file: one include line is added to their config the first time.

## Settings file

Everything lives in `~/.config/dynamic-larpland/config.toml`. The settings window writes it, and edits made by hand
apply right away. The notification history is in `~/.local/state/dynamic-larpland/notifications.json`.

## Troubleshooting

- **No notifications show up.** Another daemon owns them. Quit mako, dunst or swaync, or turn off your shell's
  notifications, then restart Dynamic-Larpland.
- **Keybinds do nothing on Hyprland.** Make sure your `hyprland.lua` has the `binds.lua` line from above.
- **Apps aren't recolored.** Tick them in **Settings → Look & theme** and press **Apply now**.

## Contributing

Develop inside a nested Hyprland window, so your own desktop and shell are never touched:

```bash
./scripts/dev.sh
```

It builds the project and opens a window with its own session, demo wallpapers and a separate config in `.dev/`.
`dev/fake-player.py` and `dev/fake-tray.py` add a demo player and a tray item. Read `CLAUDE.md` for the layout and the
rules.

## Credits

Dynamic-Larpland's UI, rendering and Wayland layer is [Noctalia](https://github.com/noctalia-dev/noctalia-shell)'s
`noctalia-ui` library (MIT), together with its theme engine, app templates and several services. See
[CREDITS.md](CREDITS.md) for the full list.
