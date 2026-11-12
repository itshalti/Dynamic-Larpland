#pragma once

#include "island/page_context.h"

// Media card for the expanded page: cover, title, artist, album, prev / play-pause / next.
// Clicking the cover or the text opens the full player.
void buildMediaCard(Node& parent, const PageContext& ctx, float x, float y, float width, float height);

// Full player: big cover (click = back), title/artist, progress bar with elapsed/remaining time and click/drag
// seek, prev / play-pause / next.
BuiltPage buildPlayerPage(const PageContext& ctx, float width, float height);
