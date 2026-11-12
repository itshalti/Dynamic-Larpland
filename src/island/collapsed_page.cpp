#include "island/collapsed_page.h"

#include "island/theme.h"
#include "island/widgets.h"
#include "render/animation/animation_manager.h"
#include "render/core/async_texture_cache.h"
#include "services/media_service.h"
#include "services/notification_service.h"
#include "ui/controls/box.h"
#include "ui/controls/glyph.h"
#include "ui/controls/image.h"
#include "ui/controls/label.h"

#include <array>
#include <random>

namespace {

  constexpr float kThumb = 22.0F;
  constexpr int kBars = 4;
  constexpr float kBarWidth = 2.5F;
  constexpr float kBarMax = 14.0F;

  void thumb(Node& parent, const PageContext& ctx, const std::string& path, float x, float y, const char* fallback) {
    auto tile = std::make_unique<Box>();
    tile->setPosition(x, y);
    tile->setSize(kThumb, kThumb);
    tile->setRadius(kThumb / 2.0F);
    tile->setFill(theme::surfaceRaised);
    auto* node = parent.addChild(std::move(tile));
    if (path.empty()) {
      Glyph* icon = widgets::glyph(*node, ctx.renderer, fallback, 12.0F, theme::textSecondary);
      icon->setPosition((kThumb - icon->width()) / 2.0F, (kThumb - icon->height()) / 2.0F);
      return;
    }
    auto image = std::make_unique<Image>();
    image->setSize(kThumb, kThumb);
    image->setFit(ImageFit::Cover);
    image->setRadius(kThumb / 2.0F);
    auto* img = static_cast<Image*>(node->addChild(std::move(image)));
    img->setSourceFileAsync(ctx.renderer, ctx.textures, path, static_cast<int>(kThumb * 2));
    img->setAsyncReadyCallback(ctx.requestRedraw);
  }

} // namespace

BuiltPage buildCollapsedPage(
    const PageContext& ctx, const std::string& time, const NotificationEntry* popup, float width, float height
) {
  auto page = std::make_unique<Node>();
  page->setSize(width, height);

  if (popup != nullptr) {
    thumb(*page, ctx, popup->icon, 7.0F, (height - kThumb) / 2.0F, "bell");
    Label* app = widgets::label(*page, ctx.renderer, popup->appName, theme::smallFontSize, theme::textSecondary,
                                false, 110.0F);
    app->setPosition(width - 16.0F - app->width(), (height - app->height()) / 2.0F);
    const float textX = 7.0F + kThumb + 9.0F;
    Label* summary = widgets::label(
        *page, ctx.renderer, popup->summary.empty() ? popup->appName : popup->summary, theme::smallFontSize + 1.0F,
        theme::textPrimary, true, app->x() - textX - 8.0F
    );
    summary->setPosition(textX, (height - summary->height()) / 2.0F);
    return {.node = std::move(page), .tick = {}};
  }

  Label* clock = widgets::label(*page, ctx.renderer, time, theme::clockFontSize, theme::textPrimary, true);
  clock->setPosition((width - clock->width()) / 2.0F, (height - clock->height()) / 2.0F);
  if (!ctx.media.playing()) {
    return {.node = std::move(page), .tick = {}};
  }

  // Playing: cover on the left, equalizer bars on the right.
  thumb(*page, ctx, ctx.media.artPath(), 7.0F, (height - kThumb) / 2.0F, "music");
  std::array<Box*, kBars> bars{};
  const float barsX = width - 14.0F - kBars * kBarWidth - (kBars - 1) * kBarWidth * 0.8F;
  for (int i = 0; i < kBars; ++i) {
    auto bar = std::make_unique<Box>();
    bar->setSize(kBarWidth, kBarWidth);
    bar->setRadius(kBarWidth / 2.0F);
    bar->setFill(theme::accentColor(ctx.wallpapers));
    bar->setPosition(barsX + static_cast<float>(i) * kBarWidth * 1.8F, (height - kBarWidth) / 2.0F);
    bars[static_cast<std::size_t>(i)] = static_cast<Box*>(page->addChild(std::move(bar)));
  }
  // shortcut: pseudo-random levels, not the real audio (cava support can come later).
  auto tick = [bars, height, &animations = ctx.animations, redraw = ctx.requestRedraw] {
    static std::minstd_rand rng{std::random_device{}()};
    std::uniform_real_distribution<float> level(0.25F, 1.0F);
    for (Box* bar : bars) {
      const float from = bar->height();
      const float to = std::max(kBarWidth, level(rng) * kBarMax);
      animations.cancelForOwner(bar);
      animations.animate(
          from, to, 240.0F, Easing::EaseOutQuad,
          [bar, height, redraw](float h) {
            bar->setSize(kBarWidth, h);
            bar->setPosition(bar->x(), (height - h) / 2.0F);
            redraw();
          },
          {}, bar
      );
    }
  };
  return {.node = std::move(page), .tick = tick};
}
