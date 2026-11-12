#include "island/tray_view.h"

#include "cursor-shape-v1-client-protocol.h"
#include "dbus/tray_service.h"
#include "island/theme.h"
#include "island/widgets.h"
#include "render/core/async_texture_cache.h"
#include "render/core/texture_manager.h"
#include "system/icon_resolver.h"
#include "ui/controls/box.h"
#include "ui/controls/image.h"

#include <filesystem>
#include <linux/input-event-codes.h>

namespace {

  constexpr float kButton = 28.0F;
  constexpr float kIcon = 18.0F;
  constexpr float kGap = 2.0F;

} // namespace

float buildTray(Node& parent, const PageContext& ctx, float x, float y, float height) {
  float cursor = x;
  for (const auto& item : ctx.tray.items()) {
    auto area = std::make_unique<InputArea>();
    area->setPosition(cursor, y + (height - kButton) / 2.0F);
    area->setSize(kButton, kButton);
    area->setCursorShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER);
    area->setAcceptedButtons(InputArea::buttonMask({BTN_LEFT, BTN_RIGHT, BTN_MIDDLE}));
    auto* button = static_cast<InputArea*>(parent.addChild(std::move(area)));

    auto bg = std::make_unique<Box>();
    bg->setSize(kButton, kButton);
    bg->setRadius(kButton / 2.0F);
    bg->setFill(Color{0, 0, 0, 0});
    auto* background = static_cast<Box*>(button->addChild(std::move(bg)));

    auto image = std::make_unique<Image>();
    image->setPosition((kButton - kIcon) / 2.0F, (kButton - kIcon) / 2.0F);
    image->setSize(kIcon, kIcon);
    image->setFit(ImageFit::Contain);
    auto* img = static_cast<Image*>(button->addChild(std::move(image)));
    // Prefer a themed icon name; fall back to the pixmap the item sent.
    std::string path;
    if (!item.iconName.empty()) {
      path = item.iconName.front() == '/' ? item.iconName : ctx.icons.resolve(item.iconName, 32);
      if (path.empty() && !item.iconThemePath.empty()) {
        for (const char* ext : {".png", ".svg"}) {
          const std::string candidate = item.iconThemePath + "/" + item.iconName + ext;
          if (std::filesystem::exists(candidate)) {
            path = candidate;
            break;
          }
        }
      }
    }
    if (!path.empty()) {
      img->setSourceFileAsync(ctx.renderer, ctx.textures, path, 48);
      img->setAsyncReadyCallback(ctx.requestRedraw);
    } else if (!item.iconArgb32.empty() && item.iconWidth > 0 && item.iconHeight > 0) {
      img->setSourceRaw(ctx.renderer, item.iconArgb32.data(), item.iconArgb32.size(), item.iconWidth,
                        item.iconHeight, item.iconWidth * 4, PixmapFormat::ARGB);
    }

    button->setOnEnter([background, redraw = ctx.requestRedraw](const InputArea::PointerData&) {
      background->setFill(theme::surfaceHover);
      redraw();
    });
    button->setOnLeave([background, redraw = ctx.requestRedraw] {
      background->setFill(Color{0, 0, 0, 0});
      redraw();
    });
    const float anchorX = cursor;
    const float anchorY = button->y();
    // ctx is a temporary: capture the service and the callback, not ctx itself.
    button->setOnClick([&tray = ctx.tray, open = ctx.openTrayMenu, id = item.id, anchorX,
                        anchorY](const InputArea::PointerData& data) {
      if (data.button == BTN_RIGHT) {
        open(id, anchorX, anchorY, kButton, kButton);
      } else if (data.button == BTN_LEFT && !tray.activateItem(id) && tray.itemUsesDBusMenu(id)) {
        // Menu-only items (no Activate): left click opens the menu too.
        open(id, anchorX, anchorY, kButton, kButton);
      }
    });
    cursor += kButton + kGap;
  }
  return cursor - x;
}
