#!/usr/bin/env bash
# Start Dynamic-Larpland with the desktop's icon theme.
# Quickshell only picks an icon theme from QS_ICON_THEME (or a pragma), not from GTK/qt*ct settings,
# so detect it here: $DYNAMIC_LARPLAND_ICON_THEME > GNOME/GTK setting > qt6ct/qt5ct > gtk settings.ini.
# Themes are looked up in ~/.local/share/icons, ~/.icons and /usr/share/icons.
set -euo pipefail

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
config="${XDG_CONFIG_HOME:-$HOME/.config}"

detect_icon_theme() {
    local theme=""
    if [[ -n "${DYNAMIC_LARPLAND_ICON_THEME:-}" ]]; then
        theme="$DYNAMIC_LARPLAND_ICON_THEME"
    elif command -v gsettings >/dev/null 2>&1; then
        theme="$(gsettings get org.gnome.desktop.interface icon-theme 2>/dev/null | tr -d "'")"
    fi
    for file in "$config/qt6ct/qt6ct.conf" "$config/qt5ct/qt5ct.conf"; do
        [[ -n "$theme" ]] && break
        [[ -f "$file" ]] && theme="$(sed -n 's/^icon_theme=//p' "$file" | head -n1)"
    done
    if [[ -z "$theme" && -f "$config/gtk-3.0/settings.ini" ]]; then
        theme="$(sed -n 's/^gtk-icon-theme-name=//p' "$config/gtk-3.0/settings.ini" | head -n1)"
    fi
    echo "${theme:-hicolor}"
}

export QS_ICON_THEME="${QS_ICON_THEME:-$(detect_icon_theme)}"
echo "dynamic-larpland: icon theme $QS_ICON_THEME"
exec qs -p "$repo" "$@"
