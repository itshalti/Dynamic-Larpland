#pragma once

#include "render/core/color.h"

// Design tokens for the island. Every color, size, radius and timing comes from here.
namespace theme {

  inline constexpr Color surface = rgbHex(0x000000);
  inline constexpr Color textPrimary = rgbHex(0xffffff);
  inline constexpr Color textSecondary = rgbHex(0x8e8e93);
  inline constexpr Color accent = rgbHex(0x3fd0b0);

  struct Size {
    float width;
    float height;
  };

  inline constexpr Size collapsedSize{116.0F, 34.0F};
  inline constexpr Size expandedSize{1040.0F, 232.0F};
  inline constexpr float expandedRadius = 36.0F;
  inline constexpr int topMargin = 6;

  // The layer surface is fixed and must fit the largest mode; input is limited to the pill.
  inline constexpr int windowWidth = 1080;
  inline constexpr int windowHeight = 420;

  inline constexpr float clockFontSize = 15.0F;

  inline constexpr float animMorphMs = 420.0F;

} // namespace theme
