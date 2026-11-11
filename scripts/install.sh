#!/usr/bin/env bash
# Symlink this repo into ~/.config/quickshell/dynamic-larpland so it runs with `qs -c dynamic-larpland`.
set -euo pipefail

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
target="${XDG_CONFIG_HOME:-$HOME/.config}/quickshell/dynamic-larpland"

mkdir -p "$(dirname "$target")"
if [[ -e "$target" && ! -L "$target" ]]; then
    echo "error: $target exists and is not a symlink; move it away first" >&2
    exit 1
fi
ln -sfn "$repo" "$target"
echo "linked $target -> $repo"
echo "run: qs -c dynamic-larpland"
