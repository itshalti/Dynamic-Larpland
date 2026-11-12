#pragma once

#include "island/page_context.h"
#include "render/scene/node.h"

#include <memory>

// Wallpaper picker page: header + a horizontal strip of previews from the wallpaper folder (the wheel scrolls
// it). Clicking a preview sets it as the wallpaper and recolors the system (WallpaperService::set).
BuiltPage buildWallpaperPage(const PageContext& ctx, float width, float height);
