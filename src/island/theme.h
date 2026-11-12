#pragma once

#include "render/core/color.h"
#include "services/wallpaper_service.h"

// Design tokens for the island. Every color, size, radius and timing comes from here.
namespace theme {

  inline constexpr Color surface = rgbHex(0x000000);
  inline constexpr Color textPrimary = rgbHex(0xffffff);
  inline constexpr Color textSecondary = rgbHex(0x8e8e93);
  inline constexpr Color textTertiary = rgbHex(0x4a4a4e);
  inline constexpr Color divider = rgbHex(0x232325);
  inline constexpr Color surfaceRaised = rgbHex(0x1c1c1e);
  inline constexpr Color surfaceHover = rgbHex(0x2a2a2c);
  inline constexpr Color track = rgbHex(0x3a3a3c);
  inline constexpr Color weekend = rgbHex(0xc46a5f);
  inline constexpr Color recording = rgbHex(0xff453a);
  inline constexpr Color accent = rgbHex(0x3fd0b0);

  // The accent follows the wallpaper palette once one was generated.
  inline Color accentColor(const WallpaperService& wallpapers) { return wallpapers.accent().value_or(accent); }
  // Text drawn on top of the accent color.
  inline Color accentText(const WallpaperService& wallpapers) {
    return wallpapers.onAccent().value_or(rgbHex(0x000000));
  }

  struct Size {
    float width;
    float height;
  };

  inline constexpr Size collapsedSize{116.0F, 34.0F};
  inline constexpr Size collapsedMediaSize{210.0F, 34.0F};
  inline constexpr Size collapsedNotifSize{360.0F, 34.0F};
  inline constexpr Size expandedSize{1040.0F, 232.0F};
  inline constexpr Size playerSize{470.0F, 196.0F};
  inline constexpr Size calendarSize{440.0F, 414.0F};
  inline constexpr Size wallpapersSize{1040.0F, 232.0F};
  inline constexpr Size powerSize{560.0F, 184.0F};
  inline constexpr float expandedRadius = 36.0F;
  inline constexpr int topMargin = 6;

  // The layer surface is fixed and must fit the largest mode; input is limited to the pill.
  inline constexpr int windowWidth = 1080;
  inline constexpr int windowHeight = 420;

  inline constexpr float clockFontSize = 15.0F;
  inline constexpr float tinyFontSize = 11.0F;
  inline constexpr float smallFontSize = 12.0F;
  inline constexpr float cardTitleFontSize = 15.0F;
  inline constexpr float bodyFontSize = 14.0F;
  inline constexpr float titleFontSize = 17.0F;
  inline constexpr float hugeFontSize = 30.0F;
  inline constexpr float lockClockFontSize = 128.0F;

  inline constexpr float radiusSmall = 8.0F;
  inline constexpr float radiusMedium = 14.0F;
  inline constexpr float radiusLarge = 20.0F;

  inline constexpr float animMorphMs = 420.0F;
  inline constexpr float animFadeMs = 160.0F;
  inline constexpr float animWallpaperMs = 600.0F;
  inline constexpr float animScrollMs = 260.0F;

  // Interaction timing. Leaving the pill within modeSwitchGraceMs of a page switch (the pill resizes
  // under the pointer) collapses only after modeSwitchCloseDelayMs.
  inline constexpr int hoverCloseDelayMs = 300;
  // How long an armed power tile waits for the confirming second click.
  inline constexpr int confirmTimeoutMs = 3000;
  inline constexpr int modeSwitchGraceMs = 1200;
  inline constexpr int modeSwitchCloseDelayMs = 1500;

} // namespace theme
