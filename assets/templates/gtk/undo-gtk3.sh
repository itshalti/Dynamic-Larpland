#!/usr/bin/env bash
set -euo pipefail

config_dir="${XDG_CONFIG_HOME:-$HOME/.config}/gtk-3.0"
config_file="$config_dir/gtk.css"
theme_file="$config_dir/dynamic-larpland.css"

if [ -f "$config_file" ]; then
    tmp_file="$(mktemp "${config_file}.tmp.XXXXXX")"
    trap 'rm -f "$tmp_file"' EXIT
    awk '!/^[[:space:]]*@import[[:space:]]+url\("dynamic-larpland\.css"\);[[:space:]]*$/' "$config_file" >"$tmp_file"
    if ! cmp -s "$config_file" "$tmp_file"; then
        cat "$tmp_file" >"$config_file"
    fi
fi

rm -f -- "$theme_file" "$config_dir/dynamic-larpland-extensions.css"
rm -rf -- "$config_dir/dynamic-larpland-extensions"
