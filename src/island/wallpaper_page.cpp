#include "island/wallpaper_page.h"

#include "cursor-shape-v1-client-protocol.h"

#include "island/theme.h"
#include "render/animation/animation_manager.h"
#include "render/core/async_texture_cache.h"
#include "render/scene/input_area.h"
#include "services/wallpaper_service.h"
#include "ui/controls/image.h"
#include "ui/controls/label.h"

#include <algorithm>
#include <filesystem>

namespace {

  constexpr float kMargin = 22.0F;
  constexpr float kStripTop = 60.0F;
  constexpr float kItemWidth = 200.0F;
  constexpr float kThumbHeight = 112.0F;
  constexpr float kItemHeight = 140.0F;
  constexpr float kSpacing = 14.0F;
  // Pixels the strip moves per wheel notch.
  constexpr float kWheelStep = 160.0F;

  // Mutable scroll state shared by the wheel handler and its animation.
  struct Scroll {
    Node* content = nullptr;
    float target = 0.0F;
    float max = 0.0F;
  };

  Label* addLabel(Node& parent, Renderer& renderer, const std::string& text, float size, const Color& color) {
    auto label = std::make_unique<Label>();
    label->setFontSize(size);
    label->setColor(color);
    label->setText(text);
    label->measure(renderer);
    return static_cast<Label*>(parent.addChild(std::move(label)));
  }

} // namespace

BuiltPage buildWallpaperPage(const PageContext& ctx, float width, float height) {
  auto page = std::make_unique<Node>();
  page->setSize(width, height);

  Label* title = addLabel(*page, ctx.renderer, "Wallpapers", theme::titleFontSize, theme::textPrimary);
  title->setPosition(kMargin + 6.0F, 20.0F);
  Label* dir = addLabel(*page, ctx.renderer, ctx.wallpapers.dirDisplay(), theme::smallFontSize, theme::textTertiary);
  dir->setPosition(title->x() + title->width() + 10.0F, title->y() + title->height() - dir->height() - 1.0F);

  // Viewport clips the strip; the content node inside it moves with the scroll offset.
  auto viewport = std::make_unique<InputArea>();
  viewport->setPosition(kMargin, kStripTop);
  viewport->setSize(width - 2.0F * kMargin, kItemHeight);
  viewport->setClipChildren(true);
  auto* view = static_cast<InputArea*>(page->addChild(std::move(viewport)));
  auto* content = view->addChild(std::make_unique<Node>());

  const auto& images = ctx.wallpapers.images();
  if (images.empty()) {
    Label* empty = addLabel(*page, ctx.renderer, "No images in " + ctx.wallpapers.dirDisplay(), theme::bodyFontSize,
                            theme::textSecondary);
    empty->setPosition((width - empty->width()) / 2.0F, kStripTop + (kItemHeight - empty->height()) / 2.0F);
  }

  const std::string& current = ctx.wallpapers.current();
  float x = 0.0F;
  for (const auto& path : images) {
    auto item = std::make_unique<InputArea>();
    item->setPosition(x, 0.0F);
    item->setSize(kItemWidth, kItemHeight);
    auto* area = static_cast<InputArea*>(content->addChild(std::move(item)));

    auto image = std::make_unique<Image>();
    image->setSize(kItemWidth, kThumbHeight);
    image->setFit(ImageFit::Cover);
    image->setRadius(theme::radiusMedium);
    const bool isCurrent = path == current;
    if (isCurrent) {
      image->setBorder(theme::accentColor(ctx.wallpapers), 2.0F);
    }
    auto* img = static_cast<Image*>(area->addChild(std::move(image)));
    // Decode a small preview, not the full-size wallpaper.
    img->setSourceFileAsync(ctx.renderer, ctx.textures, path, 400);
    img->setAsyncReadyCallback(ctx.requestRedraw);

    Label* name = addLabel(*area, ctx.renderer, std::filesystem::path(path).stem().string(), theme::smallFontSize,
                           isCurrent ? theme::textPrimary : theme::textSecondary);
    name->setMaxWidth(kItemWidth);
    name->measure(ctx.renderer);
    name->setPosition((kItemWidth - name->width()) / 2.0F, kThumbHeight + 6.0F);

    area->setCursorShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER);
    area->setOnClick([&wallpapers = ctx.wallpapers, path](const InputArea::PointerData&) { wallpapers.set(path); });
    area->setOnEnter([img, isCurrent, redraw = ctx.requestRedraw](const InputArea::PointerData&) {
      if (!isCurrent) {
        img->setBorder(theme::textSecondary, 1.0F);
        redraw();
      }
    });
    area->setOnLeave([img, isCurrent, redraw = ctx.requestRedraw] {
      if (!isCurrent) {
        img->setBorder(theme::surface, 0.0F);
        redraw();
      }
    });
    x += kItemWidth + kSpacing;
  }
  content->setSize(std::max(0.0F, x - kSpacing), kItemHeight);

  // Wheel (either axis) scrolls the strip sideways, eased.
  auto scroll = std::make_shared<Scroll>();
  scroll->content = content;
  scroll->max = std::max(0.0F, content->width() - view->width());
  view->setOnAxisHandler([scroll, &animations = ctx.animations, redraw = ctx.requestRedraw](
                             const InputArea::PointerData& data
                         ) {
    const float steps = data.scrollSteps() != 0.0F ? data.scrollSteps() : data.scrollDelta(kWheelStep) / kWheelStep;
    scroll->target = std::clamp(scroll->target + steps * kWheelStep, 0.0F, scroll->max);
    animations.cancelForOwner(scroll->content);
    animations.animate(
        -scroll->content->x(), scroll->target, theme::animScrollMs, Easing::EaseOutCubic,
        [scroll, redraw](float v) {
          scroll->content->setPosition(-v, 0.0F);
          redraw();
        },
        {}, scroll->content
    );
    return true;
  });
  return {.node = std::move(page), .tick = {}};
}
