#pragma once

#include "island/page_context.h"

#include <chrono>

// Calendar state shared by the week strip and the month page (kept by Islands across page rebuilds).
struct CalendarState {
  // Days the week strip is spun away from today (wheel).
  int stripOffset = 0;
  // Set by a wheel step: the rebuilt day row slides in from this x offset.
  float pendingSlide = 0.0F;
  // Month shown on the month page, and the selected day.
  std::chrono::year_month shownMonth{};
  std::chrono::year_month_day selected{};
};

// Week strip for the expanded page: month name + a carousel of days around today; the wheel spins it one day
// per notch, clicking opens the month page.
void buildWeekStrip(Node& parent, const PageContext& ctx, CalendarState& state, float x, float y, float width,
                    float height);

// Month page: header with prev/next, weekday row, 6x7 grid (today in an accent circle, other months dimmed,
// weekends red), clickable days, footer with the selected date.
BuiltPage buildMonthPage(const PageContext& ctx, CalendarState& state, float width, float height);

// Today as a calendar date (local time).
std::chrono::year_month_day today();
