#pragma once

#include "island/page_context.h"

#include <string>

struct NotificationEntry;

// Collapsed pill content: the clock, or a compact live activity - a fresh notification (icon + title + app)
// or the playing track (cover, clock, equalizer bars).
BuiltPage buildCollapsedPage(
    const PageContext& ctx, const std::string& time, const NotificationEntry* popup, float width, float height
);
