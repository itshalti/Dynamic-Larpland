#pragma once

#include "island/calendar_pages.h"
#include "island/page_context.h"

// Expanded island (NotchNook-like). Header: Nook tab, system tray, then record / wallpapers / settings / power.
// Body: media card | week strip | notifications.
BuiltPage buildExpandedPage(const PageContext& ctx, CalendarState& calendar, float width, float height);
