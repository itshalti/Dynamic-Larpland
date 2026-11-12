#!/usr/bin/env bash
# Build Dynamic-Larpland in release mode and install it into ~/.local (no root needed):
#   ~/.local/bin/dynamic-larpland, ~/.local/share/dynamic-larpland/assets
# Pass another prefix as the first argument (e.g. /usr/local, then run as root).
set -euo pipefail

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
prefix="${1:-$HOME/.local}"
build="$repo/build-release"

if [[ -d "$build" ]]; then
    meson configure "$build" --prefix "$prefix" --buildtype release >/dev/null
else
    meson setup "$build" "$repo" --prefix "$prefix" --buildtype release
fi
ninja -C "$build"
meson install -C "$build"

bin="$prefix/bin/dynamic-larpland"
cat <<MSG

Installed $bin

Start it with Hyprland (hyprland.lua):
  hl.on("hyprland.start", function() hl.exec_cmd("$bin") end)
and load the keybinds the settings window writes:
  pcall(dofile, os.getenv("HOME") .. "/.config/dynamic-larpland/binds.lua")
MSG
