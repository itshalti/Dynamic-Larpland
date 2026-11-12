#pragma once

#include "island/page_context.h"

// Notification history column: "Notifications" + Clear, then the newest first (icon, title, text, dismiss on
// hover). The wheel scrolls the list.
void buildNotificationList(Node& parent, const PageContext& ctx, float x, float y, float width, float height);
