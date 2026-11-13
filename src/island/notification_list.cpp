#include "island/notification_list.h"

#include "cursor-shape-v1-client-protocol.h"
#include "island/theme.h"
#include "island/widgets.h"
#include "render/animation/animation_manager.h"
#include "render/core/async_texture_cache.h"
#include "services/notification_service.h"
#include "ui/controls/box.h"
#include "ui/controls/glyph.h"
#include "ui/controls/image.h"
#include "ui/controls/label.h"

#include <algorithm>

namespace {

  constexpr float kHeaderHeight = 22.0F;
  constexpr float kItemHeight = 52.0F;
  constexpr float kItemGap = 6.0F;
  constexpr float kIcon = 28.0F;

  void addIcon(Node& parent, const PageContext& ctx, const std::string& path, float x, float y) {
    if (path.empty()) {
      Glyph* bell = widgets::glyph(parent, ctx.renderer, "bell", 16.0F, theme::textSecondary);
      bell->setPosition(x + (kIcon - bell->width()) / 2.0F, y + (kIcon - bell->height()) / 2.0F);
      return;
    }
    auto image = std::make_unique<Image>();
    image->setPosition(x, y);
    image->setSize(kIcon, kIcon);
    image->setFit(ImageFit::Contain);
    image->setRadius(theme::radiusSmall);
    auto* img = static_cast<Image*>(parent.addChild(std::move(image)));
    img->setSourceFileAsync(ctx.renderer, ctx.textures, path, static_cast<int>(kIcon * 2));
    img->setAsyncReadyCallback(ctx.requestRedraw);
  }

} // namespace

void buildNotificationList(Node& parent, const PageContext& ctx, float x, float y, float width, float height) {
  auto column = std::make_unique<Node>();
  column->setPosition(x, y);
  column->setSize(width, height);
  auto* root = parent.addChild(std::move(column));

  Label* title = widgets::label(*root, ctx.renderer, ctx.dnd ? "Do not disturb" : "Notifications",
                                theme::smallFontSize, ctx.dnd ? theme::accentColor(ctx.wallpapers) : theme::textSecondary);
  title->setPosition(0.0F, (kHeaderHeight - title->height()) / 2.0F);

  const auto& history = ctx.notifications.history();
  if (history.empty()) {
    Glyph* bell = widgets::glyph(*root, ctx.renderer, "bell", 22.0F, theme::textTertiary);
    Label* none = widgets::label(*root, ctx.renderer, "No notifications", theme::smallFontSize, theme::textTertiary);
    const float top = kHeaderHeight + (height - kHeaderHeight - bell->height() - 6.0F - none->height()) / 2.0F;
    bell->setPosition((width - bell->width()) / 2.0F, top);
    none->setPosition((width - none->width()) / 2.0F, top + bell->height() + 6.0F);
    return;
  }

  Label* clear = widgets::label(*root, ctx.renderer, "Clear", theme::smallFontSize, theme::textSecondary);
  clear->setPosition(width - clear->width(), title->y());
  widgets::clickArea(*root, clear->x() - 6.0F, 0.0F, clear->width() + 12.0F, kHeaderHeight,
                     [&service = ctx.notifications] { service.clear(); });

  auto viewport = std::make_unique<InputArea>();
  viewport->setPosition(0.0F, kHeaderHeight + 4.0F);
  viewport->setSize(width, height - kHeaderHeight - 4.0F);
  viewport->setClipChildren(true);
  auto* view = static_cast<InputArea*>(root->addChild(std::move(viewport)));
  auto* content = view->addChild(std::make_unique<Node>());

  float itemY = 0.0F;
  for (const auto& entry : history) {
    auto card = std::make_unique<Box>();
    card->setPosition(0.0F, itemY);
    card->setSize(width, kItemHeight);
    card->setRadius(theme::radiusMedium);
    card->setFill(theme::surfaceRaised);
    auto* box = static_cast<Box*>(content->addChild(std::move(card)));

    addIcon(*box, ctx, entry.icon, 10.0F, (kItemHeight - kIcon) / 2.0F);
    const float textX = 10.0F + kIcon + 10.0F;
    const float textW = width - textX - 30.0F;
    Label* summary = widgets::label(
        *box, ctx.renderer, entry.summary.empty() ? entry.appName : entry.summary, theme::smallFontSize + 1.0F,
        theme::textPrimary, true, textW
    );
    Label* body = widgets::label(
        *box, ctx.renderer, entry.body.empty() ? entry.appName : entry.body, theme::smallFontSize,
        theme::textSecondary, false, textW
    );
    const float textTop = (kItemHeight - summary->height() - body->height()) / 2.0F;
    summary->setPosition(textX, textTop);
    body->setPosition(textX, textTop + summary->height());

    // Dismiss (x), shown while the card is hovered.
    auto hover = std::make_unique<InputArea>();
    hover->setSize(width, kItemHeight);
    auto* area = static_cast<InputArea*>(box->addChild(std::move(hover)));
    auto* close = widgets::iconButton(
        *area, ctx.renderer, "x", 22.0F, 12.0F,
        [&service = ctx.notifications, id = entry.id] { service.dismiss(id); }, ctx.requestRedraw,
        theme::textSecondary
    );
    close->setPosition(width - 28.0F, (kItemHeight - 22.0F) / 2.0F);
    close->setOpacity(0.0F);
    area->setOnEnter([close, redraw = ctx.requestRedraw](const InputArea::PointerData&) {
      close->setOpacity(1.0F);
      redraw();
    });
    area->setOnLeave([close, redraw = ctx.requestRedraw] {
      close->setOpacity(0.0F);
      redraw();
    });
    itemY += kItemHeight + kItemGap;
  }
  content->setSize(width, std::max(0.0F, itemY - kItemGap));

  const float maxScroll = std::max(0.0F, content->height() - view->height());
  view->setOnAxisHandler([content, maxScroll, &animations = ctx.animations, redraw = ctx.requestRedraw](
                             const InputArea::PointerData& data
                         ) {
    const float step = data.scrollSteps() != 0.0F ? data.scrollSteps() * (kItemHeight + kItemGap)
                                                   : data.scrollDelta(kItemHeight + kItemGap);
    const float target = std::clamp(-content->y() + step, 0.0F, maxScroll);
    animations.cancelForOwner(content);
    animations.animate(
        -content->y(), target, theme::animScrollMs, Easing::EaseOutCubic,
        [content, redraw](float v) {
          content->setPosition(0.0F, -v);
          redraw();
        },
        {}, content
    );
    return true;
  });
}
