#!/usr/bin/env bash
# Development session: a nested Hyprland window running the island, isolated from the host shell.
# Use this instead of running the island on your real desktop.
#
#   scripts/dev.sh               own D-Bus session: the island is the notification daemon and tray host;
#                                only players/tray items started inside the window show up
#   scripts/dev.sh --shared-bus  host D-Bus: see the host's players, but the host shell keeps notifications
#
# Inside the window (ALT is the modifier): ALT+I toggle, ALT+Space launcher, ALT+Return kitty,
# ALT+N test notification, ALT+Q close window, ALT+SHIFT+E quit. Config/state live in .dev/ (gitignored).
set -euo pipefail

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
dev="$repo/.dev"
mkdir -p "$dev/config/dynamic-larpland" "$dev/state"

# Build the island first; the nested session runs build/dynamic-larpland.
[[ -d "$repo/build" ]] || meson setup "$repo/build" "$repo"
ninja -C "$repo/build"

# Demo wallpapers, so screenshots and recordings never show personal pictures.
python3 "$repo/dev/make-wallpapers.py" "$dev/wallpapers"
config="$dev/config/dynamic-larpland/config.toml"
if [[ ! -f "$config" ]]; then
    cat >"$config" <<TOML
[wallpaper]
dir = '$dev/wallpapers'
current = '$dev/wallpapers/aurora.png'

[recorder]
dir = '$dev/videos'
TOML
fi

# The icon theme comes from the real desktop settings, read before the session overrides XDG_CONFIG_HOME.
if [[ -z "${DYNAMIC_LARPLAND_ICON_THEME:-}" ]] && command -v gsettings >/dev/null 2>&1; then
    DYNAMIC_LARPLAND_ICON_THEME="$(gsettings get org.gnome.desktop.interface icon-theme 2>/dev/null | tr -d "'")"
fi
export DYNAMIC_LARPLAND_ICON_THEME DYNAMIC_LARPLAND_REPO="$repo"

if [[ "${1:-}" == "--shared-bus" ]]; then
    exec Hyprland -c "$repo/dev/hyprland-nested.lua"
fi
exec dbus-run-session -- Hyprland -c "$repo/dev/hyprland-nested.lua"
