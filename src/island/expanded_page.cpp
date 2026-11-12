#include "island/expanded_page.h"

#include "island/media_pages.h"
#include "island/notification_list.h"
#include "services/recorder_service.h"
#include "island/theme.h"
#include "island/tray_view.h"
#include "island/widgets.h"
#include "ui/controls/box.h"
#include "ui/controls/glyph.h"
#include "ui/controls/label.h"

#include <array>
#include <utility>

namespace {

  constexpr float kSideMargin = 26.0F;
  constexpr float kHeaderTop = 16.0F;
  constexpr float kHeaderHeight = 30.0F;
  constexpr float kBodyTop = 60.0F;
  constexpr float kBodyBottom = 20.0F;
  constexpr float kMediaWidth = 320.0F;
  constexpr float kNotificationsWidth = 260.0F;
  constexpr float kGap = 18.0F;

  void divider(Node& parent, float x, float y, float height) {
    auto line = std::make_unique<Box>();
    line->setPosition(x, y);
    line->setSize(1.0F, height);
    line->setFill(theme::divider);
    parent.addChild(std::move(line));
  }

  void header(Node& page, const PageContext& ctx, float width) {
    // "Nook" tab
    auto tab = std::make_unique<Box>();
    tab->setPosition(kSideMargin, kHeaderTop);
    tab->setFill(theme::surfaceRaised);
    auto* tabNode = static_cast<Box*>(page.addChild(std::move(tab)));
    Glyph* icon = widgets::glyph(*tabNode, ctx.renderer, "music", 14.0F, theme::textPrimary);
    Label* text = widgets::label(*tabNode, ctx.renderer, "Nook", theme::bodyFontSize, theme::textPrimary, true);
    icon->setPosition(12.0F, (kHeaderHeight - icon->height()) / 2.0F);
    text->setPosition(icon->x() + icon->width() + 7.0F, (kHeaderHeight - text->height()) / 2.0F);
    tabNode->setSize(text->x() + text->width() + 14.0F, kHeaderHeight);
    tabNode->setRadius(kHeaderHeight / 2.0F);
    buildTray(page, ctx, tabNode->x() + tabNode->width() + 8.0F, kHeaderTop, kHeaderHeight);

    // Right side: record, wallpapers, settings, power.
    constexpr std::array<std::pair<const char*, const char*>, 4> kButtons = {{
        {"player-record", "record"},
        {"photo", "wallpapers"},
        {"settings", "settings"},
        {"power", "power"},
    }};
    constexpr float kButton = 32.0F;
    float x = width - 16.0F - kButton * static_cast<float>(kButtons.size()) - 4.0F * (kButtons.size() - 1);
    for (const auto& [glyphName, actionName] : kButtons) {
      // While recording, the record button is a red stop button.
      const bool recording = std::string_view(actionName) == "record" && ctx.recorder.recording();
      if (recording) {
        auto dot = std::make_unique<Box>();
        dot->setPosition(x, kHeaderTop - 1.0F);
        dot->setSize(kButton, kButton);
        dot->setRadius(kButton / 2.0F);
        dot->setFill(theme::recording);
        page.addChild(std::move(dot));
      }
      auto* button = widgets::iconButton(
          page, ctx.renderer, recording ? "player-stop-filled" : glyphName, kButton, recording ? 14.0F : 18.0F,
          [action = ctx.action, name = std::string(actionName)] { action(name); }, ctx.requestRedraw
      );
      button->setPosition(x, kHeaderTop - 1.0F);
      x += kButton + 4.0F;
    }
  }

} // namespace

BuiltPage buildExpandedPage(const PageContext& ctx, CalendarState& calendar, float width, float height) {
  auto page = std::make_unique<Node>();
  page->setSize(width, height);
  header(*page, ctx, width);

  const float bodyH = height - kBodyTop - kBodyBottom;
  float x = kSideMargin;
  buildMediaCard(*page, ctx, x, kBodyTop, kMediaWidth, bodyH);
  x += kMediaWidth + kGap;
  divider(*page, x, kBodyTop + 6.0F, bodyH - 12.0F);
  x += kGap;

  const float notificationsX = width - kSideMargin - kNotificationsWidth;
  const float stripW = notificationsX - kGap - x - kGap;
  buildWeekStrip(*page, ctx, calendar, x, kBodyTop, stripW, bodyH);
  divider(*page, notificationsX - kGap, kBodyTop + 6.0F, bodyH - 12.0F);
  buildNotificationList(*page, ctx, notificationsX, kBodyTop - 4.0F, kNotificationsWidth, bodyH + 8.0F);
  return {.node = std::move(page), .tick = {}};
}
