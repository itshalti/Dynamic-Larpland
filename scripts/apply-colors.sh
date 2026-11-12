#!/usr/bin/env bash
# Run after matugen rendered the wallpaper colors: make GTK and kitty load the generated files
# (one include line each, added once) and reload what can be reloaded live.
set -u
cfg="${XDG_CONFIG_HOME:-$HOME/.config}"

# add_line FILE LINE: append LINE unless FILE already has it (creates FILE).
add_line() {
    mkdir -p "$(dirname "$1")"
    grep -qxF "$2" "$1" 2>/dev/null || printf '\n%s\n' "$2" >>"$1"
}

for v in 3.0 4.0; do
    [ -f "$cfg/gtk-$v/dynamic-larpland.css" ] && add_line "$cfg/gtk-$v/gtk.css" '@import url("dynamic-larpland.css");'
done

if [ -f "$cfg/kitty/themes/dynamic-larpland.conf" ] && command -v kitty >/dev/null; then
    add_line "$cfg/kitty/kitty.conf" 'include themes/dynamic-larpland.conf'
    pkill -USR1 -x kitty || true
fi

# binds.lua loads the generated colors.lua.
[ -n "${HYPRLAND_INSTANCE_SIGNATURE:-}" ] && hyprctl reload config-only >/dev/null
exit 0
