#include "island/power_page.h"

#include "cursor-shape-v1-client-protocol.h"
#include "core/timer_manager.h"
#include "island/theme.h"
#include "island/widgets.h"
#include "ui/controls/box.h"
#include "ui/controls/glyph.h"
#include "ui/controls/label.h"

#include <array>
#include <memory>

namespace {

  struct Tile {
    const char* glyph;
    const char* label;
    const char* action;
    bool confirm;
  };

  constexpr std::array<Tile, 5> kTiles = {{
      {"power", "Shut down", "system:poweroff", true},
      {"refresh", "Reboot", "system:reboot", true},
      {"cpu", "UEFI", "system:firmware", true},
      {"lock", "Lock", "lock", false},
      {"moon", "Sleep", "system:suspend", false},
  }};

  constexpr float kTile = 92.0F;
  constexpr float kGap = 12.0F;

  // Armed state of one tile; the timer disarms it.
  struct Armed {
    bool armed = false;
    Timer timer;
  };

} // namespace

BuiltPage buildPowerPage(const PageContext& ctx, float width, float height) {
  auto page = std::make_unique<Node>();
  page->setSize(width, height);

  auto* back = widgets::iconButton(*page, ctx.renderer, "chevron-left", 30.0F, 16.0F,
                                   [action = ctx.action] { action("expanded"); }, ctx.requestRedraw);
  back->setPosition(18.0F, 16.0F);
  Label* title = widgets::label(*page, ctx.renderer, "Power", theme::titleFontSize, theme::textPrimary, true);
  title->setPosition(back->x() + 38.0F, back->y() + (30.0F - title->height()) / 2.0F);

  const float rowW = kTiles.size() * kTile + (kTiles.size() - 1) * kGap;
  float x = (width - rowW) / 2.0F;
  const float y = height - kTile - 24.0F;
  for (const Tile& tile : kTiles) {
    auto area = std::make_unique<InputArea>();
    area->setPosition(x, y);
    area->setSize(kTile, kTile);
    area->setCursorShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER);
    auto* button = static_cast<InputArea*>(page->addChild(std::move(area)));

    auto bg = std::make_unique<Box>();
    bg->setSize(kTile, kTile);
    bg->setRadius(theme::radiusLarge);
    bg->setFill(theme::surfaceRaised);
    auto* background = static_cast<Box*>(button->addChild(std::move(bg)));

    Glyph* icon = widgets::glyph(*button, ctx.renderer, tile.glyph, 26.0F, theme::textPrimary);
    Label* label = widgets::label(*button, ctx.renderer, tile.label, theme::smallFontSize, theme::textPrimary);
    const float contentH = icon->height() + 8.0F + label->height();
    icon->setPosition((kTile - icon->width()) / 2.0F, (kTile - contentH) / 2.0F);
    label->setPosition((kTile - label->width()) / 2.0F, icon->y() + icon->height() + 8.0F);

    auto state = std::make_shared<Armed>();
    Renderer& renderer = ctx.renderer;
    auto paint = [state, background, label, &renderer, text = std::string(tile.label), redraw = ctx.requestRedraw](
                     bool hovered
                 ) {
      background->setFill(state->armed ? theme::recording : hovered ? theme::surfaceHover : theme::surfaceRaised);
      label->setText(state->armed ? "Confirm?" : text);
      label->measure(renderer);
      label->setPosition((kTile - label->width()) / 2.0F, label->y());
      redraw();
    };
    button->setOnEnter([paint](const InputArea::PointerData&) { paint(true); });
    button->setOnLeave([paint] { paint(false); });
    button->setOnClick([state, paint, confirm = tile.confirm, name = std::string(tile.action),
                        action = ctx.action](const InputArea::PointerData&) {
      if (confirm && !state->armed) {
        state->armed = true;
        paint(true);
        state->timer.start(std::chrono::milliseconds(theme::confirmTimeoutMs), [state, paint] {
          state->armed = false;
          paint(false);
        });
        return;
      }
      state->armed = false;
      state->timer.stop();
      paint(false);
      action("collapsed");
      action(name);
    });
    x += kTile + kGap;
  }
  return {.node = std::move(page), .tick = {}};
}
