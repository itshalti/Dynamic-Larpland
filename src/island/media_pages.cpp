#include "island/media_pages.h"

#include "dbus/mpris/mpris_art.h"
#include "island/theme.h"
#include "island/widgets.h"
#include "render/core/async_texture_cache.h"
#include "services/media_service.h"
#include "ui/controls/box.h"
#include "ui/controls/glyph.h"
#include "ui/controls/image.h"
#include "ui/controls/label.h"

#include <algorithm>

namespace {

  // Rounded cover tile; a music note when there is no art (yet).
  Node* cover(Node& parent, const PageContext& ctx, float x, float y, float size, float radius) {
    auto tile = std::make_unique<Box>();
    tile->setPosition(x, y);
    tile->setSize(size, size);
    tile->setRadius(radius);
    tile->setFill(theme::surfaceRaised);
    auto* node = parent.addChild(std::move(tile));

    const std::string art = ctx.media.artPath();
    if (!art.empty()) {
      auto image = std::make_unique<Image>();
      image->setSize(size, size);
      image->setFit(ImageFit::Cover);
      image->setRadius(radius);
      auto* img = static_cast<Image*>(node->addChild(std::move(image)));
      img->setSourceFileAsync(ctx.renderer, ctx.textures, art, static_cast<int>(size * 2));
      img->setAsyncReadyCallback(ctx.requestRedraw);
    } else {
      Glyph* note = widgets::glyph(*node, ctx.renderer, "music", size * 0.4F, theme::textTertiary);
      note->setPosition((size - note->width()) / 2.0F, (size - note->height()) / 2.0F);
    }
    return node;
  }

  // prev / play-pause / next row; returns its width.
  float controls(Node& parent, const PageContext& ctx, float x, float y, float small, float big) {
    MprisService& mpris = ctx.media.mpris();
    auto* prev = widgets::iconButton(
        parent, ctx.renderer, "player-skip-back-filled", small, small * 0.5F, [&mpris] { mpris.previousActive(); },
        ctx.requestRedraw
    );
    prev->setPosition(x, y + (big - small) / 2.0F);
    auto* play = widgets::iconButton(
        parent, ctx.renderer, ctx.media.playing() ? "player-pause-filled" : "player-play-filled", big, big * 0.5F,
        [&mpris] { mpris.playPauseActive(); }, ctx.requestRedraw
    );
    play->setPosition(x + small + 6.0F, y);
    auto* next = widgets::iconButton(
        parent, ctx.renderer, "player-skip-forward-filled", small, small * 0.5F, [&mpris] { mpris.nextActive(); },
        ctx.requestRedraw
    );
    next->setPosition(x + small + big + 12.0F, y + (big - small) / 2.0F);
    return 2.0F * small + big + 12.0F;
  }

} // namespace

void buildMediaCard(Node& parent, const PageContext& ctx, float x, float y, float width, float height) {
  auto card = std::make_unique<Node>();
  card->setPosition(x, y);
  card->setSize(width, height);
  auto* root = parent.addChild(std::move(card));

  const auto player = ctx.media.active();
  if (!player) {
    Glyph* note = widgets::glyph(*root, ctx.renderer, "music", 26.0F, theme::textTertiary);
    Label* text = widgets::label(*root, ctx.renderer, "Nothing playing", theme::bodyFontSize, theme::textSecondary);
    const float total = note->height() + 8.0F + text->height();
    note->setPosition((width - note->width()) / 2.0F, (height - total) / 2.0F);
    text->setPosition((width - text->width()) / 2.0F, note->y() + note->height() + 8.0F);
    return;
  }

  constexpr float kCover = 96.0F;
  const float coverY = (height - kCover) / 2.0F;
  cover(*root, ctx, 0.0F, coverY, kCover, theme::radiusMedium);

  const float textX = kCover + 16.0F;
  const float textW = width - textX;
  Label* title = widgets::label(
      *root, ctx.renderer, player->title.empty() ? "Unknown title" : player->title, theme::cardTitleFontSize,
      theme::textPrimary, true, textW
  );
  title->setPosition(textX, coverY - 4.0F);
  Label* artist = widgets::label(
      *root, ctx.renderer, mpris::joinArtists(player->artists), theme::bodyFontSize - 1.0F, theme::textSecondary,
      false, textW
  );
  artist->setPosition(textX, title->y() + title->height() + 1.0F);
  if (!player->album.empty()) {
    Label* album =
        widgets::label(*root, ctx.renderer, player->album, theme::smallFontSize, theme::textTertiary, false, textW);
    album->setPosition(textX, artist->y() + artist->height());
  }
  // Cover and text open the full player; the buttons below sit on top and keep their own clicks.
  widgets::clickArea(*root, 0.0F, coverY, width, kCover - 36.0F, [action = ctx.action] { action("player"); });
  controls(*root, ctx, textX - 6.0F, coverY + kCover - 34.0F, 30.0F, 34.0F);
}

BuiltPage buildPlayerPage(const PageContext& ctx, float width, float height) {
  auto page = std::make_unique<Node>();
  page->setSize(width, height);
  const auto player = ctx.media.active();

  constexpr float kPad = 24.0F;
  constexpr float kCover = 132.0F;
  const float coverY = (height - kCover) / 2.0F;
  cover(*page, ctx, kPad, coverY, kCover, theme::radiusLarge);
  widgets::clickArea(*page, kPad, coverY, kCover, kCover, [action = ctx.action] { action("expanded"); });

  const float colX = kPad + kCover + 20.0F;
  const float colW = width - colX - kPad;
  if (!player) {
    Label* none = widgets::label(*page, ctx.renderer, "Nothing playing", theme::bodyFontSize, theme::textSecondary);
    none->setPosition(colX, (height - none->height()) / 2.0F);
    return {.node = std::move(page), .tick = {}};
  }

  Label* title = widgets::label(
      *page, ctx.renderer, player->title.empty() ? "Unknown title" : player->title, theme::titleFontSize,
      theme::textPrimary, true, colW
  );
  title->setPosition(colX, coverY + 2.0F);
  Label* artist = widgets::label(
      *page, ctx.renderer, mpris::joinArtists(player->artists), theme::bodyFontSize, theme::textSecondary, false, colW
  );
  artist->setPosition(colX, title->y() + title->height() + 2.0F);

  // Progress bar with click/drag seek.
  const float barY = coverY + 68.0F;
  auto trackBox = std::make_unique<Box>();
  trackBox->setPosition(colX, barY);
  trackBox->setSize(colW, 4.0F);
  trackBox->setRadius(2.0F);
  trackBox->setFill(theme::track);
  page->addChild(std::move(trackBox));
  auto fillBox = std::make_unique<Box>();
  fillBox->setPosition(colX, barY);
  fillBox->setSize(0.0F, 4.0F);
  fillBox->setRadius(2.0F);
  fillBox->setFill(theme::accentColor(ctx.wallpapers));
  auto* fill = static_cast<Box*>(page->addChild(std::move(fillBox)));

  Label* elapsed = widgets::label(*page, ctx.renderer, "0:00", theme::tinyFontSize, theme::textSecondary);
  elapsed->setPosition(colX, barY + 10.0F);
  Label* remaining = widgets::label(*page, ctx.renderer, "-0:00", theme::tinyFontSize, theme::textSecondary);
  remaining->setPosition(colX + colW - remaining->width(), barY + 10.0F);

  MediaService& media = ctx.media;
  Renderer& renderer = ctx.renderer;
  const std::int64_t length = player->lengthUs;
  auto tick = [&media, &renderer, fill, elapsed, remaining, colX, colW, length, redraw = ctx.requestRedraw] {
    const std::int64_t pos = std::clamp<std::int64_t>(media.position(), 0, std::max<std::int64_t>(length, 0));
    const float progress = length > 0 ? static_cast<float>(pos) / static_cast<float>(length) : 0.0F;
    fill->setSize(colW * progress, 4.0F);
    elapsed->setText(widgets::formatDuration(pos));
    elapsed->measure(renderer);
    remaining->setText("-" + widgets::formatDuration(length - pos));
    remaining->measure(renderer);
    remaining->setPosition(colX + colW - remaining->width(), remaining->y());
    redraw();
  };
  tick();

  if (player->canSeek && length > 0) {
    auto seek = std::make_unique<InputArea>();
    seek->setPosition(colX, barY - 8.0F);
    seek->setSize(colW, 20.0F);
    auto* area = static_cast<InputArea*>(page->addChild(std::move(seek)));
    auto seekTo = [&media, colW, length, tick](float localX) {
      const double fraction = std::clamp(static_cast<double>(localX / colW), 0.0, 1.0);
      media.mpris().setPositionActive(static_cast<std::int64_t>(fraction * static_cast<double>(length)));
      tick();
    };
    area->setOnPress([seekTo](const InputArea::PointerData& data) { seekTo(data.localX); });
    // Drag: motion events while the button is held keep seeking.
    area->setOnMotion([seekTo, area](const InputArea::PointerData& data) {
      if (area->pressed()) {
        seekTo(data.localX);
      }
    });
  }

  controls(*page, ctx, colX + (colW - (2.0F * 36.0F + 44.0F + 12.0F)) / 2.0F, barY + 30.0F, 36.0F, 44.0F);
  return {.node = std::move(page), .tick = tick};
}
