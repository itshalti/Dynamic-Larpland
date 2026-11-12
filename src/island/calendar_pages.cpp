#include "island/calendar_pages.h"

#include "cursor-shape-v1-client-protocol.h"

#include "island/theme.h"
#include "island/widgets.h"
#include "render/animation/animation_manager.h"
#include "ui/controls/box.h"
#include "ui/controls/glyph.h"
#include "ui/controls/label.h"

#include <array>
#include <ctime>
#include <string>

namespace {

  using namespace std::chrono;

  constexpr std::array<const char*, 12> kMonths = {"January", "February", "March",     "April",   "May",      "June",
                                                   "July",    "August",   "September", "October", "November", "December"};
  constexpr std::array<const char*, 7> kWeekdays = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  constexpr std::array<const char*, 7> kWeekdayLetters = {"S", "M", "T", "W", "T", "F", "S"};

  std::string monthName(month m) { return kMonths[static_cast<unsigned>(m) - 1]; }

  bool isWeekend(weekday wd) { return wd == Sunday || wd == Saturday; }

  year_month_day addDays(year_month_day d, int days) { return year_month_day{sys_days{d} + std::chrono::days{days}}; }

} // namespace

std::chrono::year_month_day today() {
  const std::time_t now = std::time(nullptr);
  std::tm local{};
  localtime_r(&now, &local);
  return year{local.tm_year + 1900} / month{static_cast<unsigned>(local.tm_mon + 1)}
         / day{static_cast<unsigned>(local.tm_mday)};
}

void buildWeekStrip(Node& parent, const PageContext& ctx, CalendarState& state, float x, float y, float width,
                    float height) {
  auto strip = std::make_unique<InputArea>();
  strip->setPosition(x, y);
  strip->setSize(width, height);
  strip->setClipChildren(true);
  auto* root = static_cast<InputArea*>(parent.addChild(std::move(strip)));

  const year_month_day now = today();
  const year_month_day center = addDays(now, state.stripOffset);

  Label* monthLabel = widgets::label(
      *root, ctx.renderer, monthName(center.month()).substr(0, 3), theme::hugeFontSize, theme::textPrimary, true
  );
  const float rowCenterY = height * 0.40F;
  monthLabel->setPosition(4.0F, rowCenterY - monthLabel->height() / 2.0F);

  // Carousel: 7 days around the focused one; the middle one is larger.
  const float daysX = monthLabel->x() + monthLabel->width() + 14.0F;
  const float colW = (width - daysX) / 7.0F;
  auto row = std::make_unique<Node>();
  row->setPosition(daysX, 0.0F);
  row->setSize(width - daysX, height);
  auto* days = root->addChild(std::move(row));
  for (int i = -3; i <= 3; ++i) {
    const year_month_day d = addDays(center, i);
    const weekday wd{sys_days{d}};
    const bool isToday = d == now;
    const bool focused = i == 0;
    const Color dayColor = isToday       ? theme::accentColor(ctx.wallpapers)
                           : isWeekend(wd) ? theme::weekend
                           : focused       ? theme::textPrimary
                                           : theme::textSecondary;
    Label* letter = widgets::label(
        *days, ctx.renderer, focused ? std::string(kWeekdays[wd.c_encoding()]) : kWeekdayLetters[wd.c_encoding()],
        theme::tinyFontSize, isWeekend(wd) ? theme::weekend : theme::textTertiary, focused
    );
    Label* number = widgets::label(
        *days, ctx.renderer, std::to_string(static_cast<unsigned>(d.day())),
        focused ? theme::titleFontSize + 5.0F : theme::titleFontSize, dayColor, focused || isToday
    );
    const float cx = (static_cast<float>(i + 3) + 0.5F) * colW;
    // Days fade towards the edges.
    const float fade = 1.0F - static_cast<float>(std::abs(i)) * 0.18F;
    letter->setOpacity(fade);
    number->setOpacity(fade);
    number->setPosition(cx - number->width() / 2.0F, rowCenterY - number->height() / 2.0F + 6.0F);
    letter->setPosition(cx - letter->width() / 2.0F, number->y() - letter->height());
  }

  Glyph* icon = widgets::glyph(*root, ctx.renderer, "calendar", 15.0F, theme::textSecondary);
  Label* empty = widgets::label(
      *root, ctx.renderer, state.stripOffset == 0 ? "Nothing for today" : "Nothing planned", theme::bodyFontSize,
      theme::textSecondary
  );
  const float footY = height * 0.78F;
  const float footW = icon->width() + 8.0F + empty->width();
  icon->setPosition((width - footW) / 2.0F, footY - icon->height() / 2.0F);
  empty->setPosition(icon->x() + icon->width() + 8.0F, footY - empty->height() / 2.0F);

  root->setCursorShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER);
  root->setOnClick([action = ctx.action](const InputArea::PointerData&) { action("calendar"); });
  // Wheel: one day per notch (down = forward), sliding the row in.
  root->setOnAxisHandler([&state, colW, action = ctx.action](const InputArea::PointerData& data) {
    const float steps = data.scrollSteps();
    if (steps == 0.0F) {
      return true;
    }
    const int step = steps > 0.0F ? 1 : -1;
    state.stripOffset += step;
    // The rebuilt row (new offset) slides in from one column over.
    state.pendingSlide = static_cast<float>(step) * colW;
    action("rebuild:expanded");
    return true;
  });
  if (state.pendingSlide != 0.0F) {
    const float from = state.pendingSlide;
    state.pendingSlide = 0.0F;
    days->setPosition(daysX + from, 0.0F);
    ctx.animations.animate(
        from, 0.0F, theme::animScrollMs, Easing::EaseOutCubic,
        [days, daysX, redraw = ctx.requestRedraw](float v) {
          days->setPosition(daysX + v, 0.0F);
          redraw();
        },
        {}, days
    );
  }
}

BuiltPage buildMonthPage(const PageContext& ctx, CalendarState& state, float width, float height) {
  auto page = std::make_unique<Node>();
  page->setSize(width, height);
  const year_month_day now = today();
  if (!state.shownMonth.ok()) {
    state.shownMonth = now.year() / now.month();
    state.selected = now;
  }
  const year_month shown = state.shownMonth;
  constexpr float kPad = 24.0F;

  // Header: Month Year, prev / next.
  Label* title = widgets::label(
      *page, ctx.renderer, monthName(shown.month()) + " " + std::to_string(static_cast<int>(shown.year())),
      theme::titleFontSize + 1.0F, theme::textPrimary, true
  );
  title->setPosition(kPad + 6.0F, 22.0F);
  auto* prev = widgets::iconButton(
      *page, ctx.renderer, "chevron-left", 30.0F, 16.0F,
      [&state, action = ctx.action] {
        state.shownMonth -= months{1};
        action("rebuild:calendar");
      },
      ctx.requestRedraw
  );
  prev->setPosition(width - kPad - 64.0F, 18.0F);
  auto* next = widgets::iconButton(
      *page, ctx.renderer, "chevron-right", 30.0F, 16.0F,
      [&state, action = ctx.action] {
        state.shownMonth += months{1};
        action("rebuild:calendar");
      },
      ctx.requestRedraw
  );
  next->setPosition(width - kPad - 30.0F, 18.0F);

  // Weekday header and the 6x7 grid (fixed cells keep columns aligned).
  const int firstDay = std::clamp(ctx.firstDayOfWeek, 0, 6);
  const float cellW = (width - 2.0F * kPad) / 7.0F;
  constexpr float kCellH = 40.0F;
  const float gridTop = 70.0F;
  for (int c = 0; c < 7; ++c) {
    const weekday wd{static_cast<unsigned>((firstDay + c) % 7)};
    Label* name = widgets::label(*page, ctx.renderer, kWeekdays[wd.c_encoding()], theme::smallFontSize,
                                 isWeekend(wd) ? theme::weekend : theme::textSecondary);
    name->setPosition(kPad + (static_cast<float>(c) + 0.5F) * cellW - name->width() / 2.0F, gridTop);
  }
  const year_month_day first = shown / day{1};
  const int lead = (static_cast<int>(weekday{sys_days{first}}.c_encoding()) - firstDay + 7) % 7;
  const year_month_day gridStart = addDays(first, -lead);
  const Color accent = theme::accentColor(ctx.wallpapers);
  for (int i = 0; i < 42; ++i) {
    const year_month_day d = addDays(gridStart, i);
    const weekday wd{sys_days{d}};
    const bool inMonth = d.year() / d.month() == shown;
    const bool isToday = d == now;
    const bool isSelected = d == state.selected;
    const float cx = kPad + (static_cast<float>(i % 7) + 0.5F) * cellW;
    const float cy = gridTop + 24.0F + (static_cast<float>(i / 7) + 0.5F) * kCellH;
    if (isToday || isSelected) {
      auto circle = std::make_unique<Box>();
      circle->setSize(32.0F, 32.0F);
      circle->setPosition(cx - 16.0F, cy - 16.0F);
      circle->setRadius(16.0F);
      if (isToday) {
        circle->setFill(accent);
      } else {
        circle->setFill(Color{0, 0, 0, 0});
        circle->setBorder(accent, 1.5F);
      }
      page->addChild(std::move(circle));
    }
    const Color color = isToday ? theme::accentText(ctx.wallpapers)
                        : !inMonth     ? theme::textTertiary
                        : isWeekend(wd) ? theme::weekend
                                        : theme::textPrimary;
    Label* number = widgets::label(*page, ctx.renderer, std::to_string(static_cast<unsigned>(d.day())),
                                   theme::bodyFontSize, color, isToday);
    number->setPosition(cx - number->width() / 2.0F, cy - number->height() / 2.0F);
    widgets::clickArea(*page, cx - cellW / 2.0F, cy - kCellH / 2.0F, cellW, kCellH, [&state, d, action = ctx.action] {
      state.selected = d;
      state.shownMonth = d.year() / d.month();
      action("rebuild:calendar");
    });
  }

  // Footer: selected date.
  const weekday selWd{sys_days{state.selected}};
  Label* footer = widgets::label(
      *page, ctx.renderer,
      std::string(kWeekdays[selWd.c_encoding()]) + ", " + std::to_string(static_cast<unsigned>(state.selected.day()))
          + " " + monthName(state.selected.month()),
      theme::bodyFontSize, theme::textPrimary, true
  );
  footer->setPosition(kPad + 6.0F, height - 52.0F);
  Label* none = widgets::label(*page, ctx.renderer, "Nothing planned", theme::smallFontSize, theme::textSecondary);
  none->setPosition(kPad + 6.0F, footer->y() + footer->height() + 2.0F);
  return {.node = std::move(page), .tick = {}};
}
