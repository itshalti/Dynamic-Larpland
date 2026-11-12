#pragma once

#include "island/page_context.h"

// System tray icons (StatusNotifierItem) for the expanded header. Left click activates an item, right click
// (or left click on menu-only items) opens its menu. Returns the row width.
float buildTray(Node& parent, const PageContext& ctx, float x, float y, float height);
