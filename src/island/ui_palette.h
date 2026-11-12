#pragma once

class WallpaperService;

// Sets noctalia-ui's palette (used by its own controls: menus, inputs, toggles) to the island style:
// black surfaces, grey text, the wallpaper accent as primary.
void applyUiPalette(const WallpaperService& wallpapers);
