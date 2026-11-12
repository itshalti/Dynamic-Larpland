#include "launcher/launcher.h"

#include "core/deferred_call.h"
#include "core/log.h"
#include "core/ui_phase.h"
#include "cursor-shape-v1-client-protocol.h"
#include "island/theme.h"
#include "island/widgets.h"
#include "launcher/fuzzy_match.h"
#include "render/core/async_texture_cache.h"
#include "system/desktop_entry.h"
#include "system/desktop_entry_launch.h"
#include "system/icon_resolver.h"
#include "ui/controls/box.h"
#include "ui/controls/glyph.h"
#include "ui/controls/image.h"
#include "ui/controls/label.h"
#include "wayland/wayland_connection.h"
#include "wayland/wayland_seat.h"

#include <algorithm>
#include <xkbcommon/xkbcommon-keysyms.h>

namespace {

  constexpr Logger kLog("launcher");
  constexpr float kCardWidth = 680.0F;
  constexpr float kRowHeight = 60.0F;
  constexpr std::size_t kMaxRows = 7;
  constexpr float kSearchHeight = 52.0F;
  constexpr float kPad = 12.0F;

  std::string lower(std::string s) {
    std::ranges::transform(s, s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
  }

  void appendUtf8(std::string& out, std::uint32_t cp) {
    if (cp < 0x80) {
      out += static_cast<char>(cp);
    } else if (cp < 0x800) {
      out += static_cast<char>(0xC0 | (cp >> 6));
      out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
      out += static_cast<char>(0xE0 | (cp >> 12));
      out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
      out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
      out += static_cast<char>(0xF0 | (cp >> 18));
      out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
      out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
      out += static_cast<char>(0x80 | (cp & 0x3F));
    }
  }

  void popUtf8(std::string& s) {
    while (!s.empty()) {
      const auto c = static_cast<unsigned char>(s.back());
      s.pop_back();
      if ((c & 0xC0) != 0x80) {
        break;
      }
    }
  }

} // namespace

void Launcher::initialize(WaylandConnection& wayland, RenderContext& renderContext, AsyncTextureCache& textures,
                          IconResolver& icons, WallpaperService& wallpapers) {
  m_wayland = &wayland;
  m_renderContext = &renderContext;
  m_textures = &textures;
  m_icons = &icons;
  m_wallpapers = &wallpapers;
}

void Launcher::toggle(const std::string& connector) {
  if (isOpen()) {
    close();
  } else {
    open(connector);
  }
}

void Launcher::open(const std::string& connector) {
  if (isOpen()) {
    return;
  }
  wl_output* output = nullptr;
  for (const auto& o : m_wayland->outputs()) {
    if (output == nullptr || o.connectorName == connector) {
      output = o.output;
    }
  }
  refreshDesktopEntriesIfSourcesChanged();
  m_text.clear();
  m_selected = 0;
  m_scroll = 0;
  filter();

  m_surface = std::make_unique<LayerSurface>(
      *m_wayland,
      LayerSurfaceConfig{
          .nameSpace = "dynamic-larpland-launcher",
          .layer = LayerShellLayer::Overlay,
          .anchor = LayerShellAnchor::Top | LayerShellAnchor::Bottom | LayerShellAnchor::Left
                    | LayerShellAnchor::Right,
          .exclusiveZone = -1,
          .keyboard = LayerShellKeyboard::Exclusive,
      }
  );
  m_surface->setRenderContext(m_renderContext);
  m_surface->setAnimationManager(&m_animations);
  m_surface->setConfigureCallback([this](std::uint32_t w, std::uint32_t h) {
    kLog.debug("launcher configured {}x{}", w, h);
    m_surface->requestLayout();
  });
  m_surface->setPrepareFrameCallback([this](bool, bool) {
    if (m_root == nullptr || m_root->width() != static_cast<float>(m_surface->width())) {
      UiPhaseScope layoutPhase(UiPhase::Layout);
      buildScene();
    }
  });
  if (!m_surface->initialize(output)) {
    kLog.warn("could not create the launcher surface");
    m_surface.reset();
  }
}

void Launcher::close() {
  if (!isOpen()) {
    return;
  }
  m_animations.cancelAll();
  m_input.setSceneRoot(nullptr);
  m_root.reset();
  m_card = m_list = nullptr;
  m_query = nullptr;
  m_surface.reset();
}

void Launcher::filter() {
  const auto& entries = desktopEntries();
  m_results.clear();
  if (m_text.empty()) {
    for (const auto& e : entries) {
      if (!e.noDisplay && !e.hidden) {
        m_results.push_back(&e);
      }
    }
    std::ranges::sort(m_results, [](const DesktopEntry* a, const DesktopEntry* b) { return a->nameLower < b->nameLower; });
    return;
  }
  const std::string query = lower(m_text);
  std::vector<std::pair<double, const DesktopEntry*>> scored;
  for (const auto& e : entries) {
    if (e.noDisplay || e.hidden) {
      continue;
    }
    // The name counts most; generic name and keywords help but rank lower.
    double best = FuzzyMatch::score(query, e.nameLower);
    for (const auto& [text, weight] : {std::pair{&e.genericNameLower, 0.6}, std::pair{&e.keywordsLower, 0.5},
                                       std::pair{&e.execLower, 0.4}}) {
      const double s = FuzzyMatch::score(query, *text);
      if (FuzzyMatch::isMatch(s)) {
        best = std::max(best, s * weight);
      }
    }
    if (FuzzyMatch::isMatch(best)) {
      // Prefix matches on the name win.
      if (e.nameLower.starts_with(query)) {
        best += 100.0;
      }
      scored.emplace_back(best, &e);
    }
  }
  std::ranges::sort(scored, [](const auto& a, const auto& b) { return a.first > b.first; });
  for (const auto& [score, e] : scored) {
    m_results.push_back(e);
  }
}

void Launcher::buildScene() {
  const auto w = static_cast<float>(m_surface->width());
  const auto h = static_cast<float>(m_surface->height());
  Renderer& renderer = m_surface->renderTarget().renderer();

  m_root = std::make_unique<Node>();
  m_root->setSize(w, h);
  // Click outside the card closes.
  auto outside = std::make_unique<InputArea>();
  outside->setSize(w, h);
  outside->setOnClick([this](const InputArea::PointerData&) { DeferredCall::callLater([this] { close(); }); });
  m_root->addChild(std::move(outside));

  auto card = std::make_unique<Box>();
  card->setFill(theme::surface);
  card->setBorder(theme::divider, 1.0F);
  card->setRadius(30.0F);
  card->setPosition((w - kCardWidth) / 2.0F, h * 0.2F);
  m_card = m_root->addChild(std::move(card));
  // Swallow clicks on the card so they don't reach the outside area.
  auto swallow = std::make_unique<InputArea>();
  m_card->addChild(std::move(swallow));

  Glyph* search = widgets::glyph(*m_card, renderer, "search", 20.0F, theme::textSecondary);
  search->setPosition(kPad + 14.0F, kPad + (kSearchHeight - search->height()) / 2.0F);
  m_query = widgets::label(*m_card, renderer, "", theme::hugeFontSize - 10.0F, theme::textPrimary);
  m_query->setPosition(search->x() + search->width() + 14.0F, 0.0F);

  auto list = std::make_unique<Node>();
  list->setPosition(kPad, kPad + kSearchHeight + 9.0F);
  m_list = m_card->addChild(std::move(list));

  m_surface->setSceneRoot(m_root.get());
  m_input.setSceneRoot(m_root.get());
  m_input.setCursorShapeCallback([this](std::uint32_t serial, std::uint32_t shape) {
    m_wayland->setCursorShape(serial, shape);
  });
  rebuildList();
}

void Launcher::rebuildList() {
  if (m_card == nullptr) {
    return;
  }
  UiPhaseScope layoutPhase(UiPhase::Layout);
  Renderer& renderer = m_surface->renderTarget().renderer();

  m_query->setText(m_text.empty() ? "Search apps" : m_text);
  m_query->setColor(m_text.empty() ? theme::textTertiary : theme::textPrimary);
  m_query->measure(renderer);
  m_query->setPosition(m_query->x(), kPad + (kSearchHeight - m_query->height()) / 2.0F);

  while (!m_list->children().empty()) {
    m_animations.cancelForOwner(m_list->children().back().get());
    m_list->removeChild(m_list->children().back().get());
  }
  const float rowW = kCardWidth - 2.0F * kPad;
  const std::size_t rows = std::min(kMaxRows, m_results.size());
  if (!m_results.empty()) {
    auto line = std::make_unique<Box>();
    line->setPosition(0.0F, -5.0F);
    line->setSize(rowW, 1.0F);
    line->setFill(theme::divider);
    m_list->addChild(std::move(line));
  }
  for (std::size_t r = 0; r < rows; ++r) {
    const std::size_t index = m_scroll + r;
    const DesktopEntry& entry = *m_results[index];
    const bool selected = index == m_selected;

    auto area = std::make_unique<InputArea>();
    area->setPosition(0.0F, static_cast<float>(r) * (kRowHeight + 4.0F));
    area->setSize(rowW, kRowHeight);
    area->setCursorShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER);
    auto* row = static_cast<InputArea*>(m_list->addChild(std::move(area)));
    if (selected) {
      auto bg = std::make_unique<Box>();
      bg->setSize(rowW, kRowHeight);
      bg->setRadius(theme::radiusMedium);
      bg->setFill(theme::surfaceRaised);
      row->addChild(std::move(bg));
      auto bar = std::make_unique<Box>();
      bar->setPosition(0.0F, 14.0F);
      bar->setSize(3.0F, kRowHeight - 28.0F);
      bar->setRadius(1.5F);
      bar->setFill(theme::accentColor(*m_wallpapers));
      row->addChild(std::move(bar));
    }
    constexpr float kIcon = 36.0F;
    const std::string& iconPath = entry.icon.empty() ? std::string() : (entry.icon.front() == '/' ? entry.icon : m_icons->resolve(entry.icon, 64));
    if (!iconPath.empty()) {
      auto image = std::make_unique<Image>();
      image->setPosition(16.0F, (kRowHeight - kIcon) / 2.0F);
      image->setSize(kIcon, kIcon);
      image->setFit(ImageFit::Contain);
      auto* img = static_cast<Image*>(row->addChild(std::move(image)));
      img->setSourceFileAsync(renderer, *m_textures, iconPath, 72);
      img->setAsyncReadyCallback([this] {
        if (m_surface) {
          m_surface->requestRedraw();
        }
      });
    } else {
      Glyph* g = widgets::glyph(*row, renderer, "apps", 24.0F, theme::textSecondary);
      g->setPosition(16.0F + (kIcon - g->width()) / 2.0F, (kRowHeight - g->height()) / 2.0F);
    }
    const float textX = 16.0F + kIcon + 14.0F;
    Label* name = widgets::label(*row, renderer, entry.name, theme::cardTitleFontSize, theme::textPrimary, true,
                                 rowW - textX - 16.0F);
    Label* generic = nullptr;
    if (!entry.genericName.empty()) {
      generic = widgets::label(*row, renderer, entry.genericName, theme::smallFontSize, theme::textSecondary, false,
                               rowW - textX - 16.0F);
    }
    const float textH = name->height() + (generic != nullptr ? generic->height() : 0.0F);
    name->setPosition(textX, (kRowHeight - textH) / 2.0F);
    if (generic != nullptr) {
      generic->setPosition(textX, name->y() + name->height());
    }
    row->setOnEnter([this, index](const InputArea::PointerData&) {
      if (m_selected != index) {
        m_selected = index;
        DeferredCall::callLater([this] { rebuildList(); });
      }
    });
    row->setOnClick([this, index](const InputArea::PointerData&) {
      DeferredCall::callLater([this, index] { launch(index); });
    });
  }
  if (m_results.empty()) {
    m_list->setSize(rowW, 0.0F);
  }
  const float listH = rows == 0 ? 0.0F : static_cast<float>(rows) * (kRowHeight + 4.0F) - 4.0F;
  const float cardH = kPad + kSearchHeight + (rows == 0 ? 0.0F : 9.0F + listH) + kPad;
  m_card->setSize(kCardWidth, cardH);
  m_card->children()[0]->setSize(kCardWidth, cardH);
  m_surface->requestRedraw();
}

void Launcher::moveSelection(int delta) {
  if (m_results.empty()) {
    return;
  }
  const auto n = static_cast<long>(m_results.size());
  m_selected = static_cast<std::size_t>(((static_cast<long>(m_selected) + delta) % n + n) % n);
  if (m_selected < m_scroll) {
    m_scroll = m_selected;
  } else if (m_selected >= m_scroll + kMaxRows) {
    m_scroll = m_selected - kMaxRows + 1;
  }
  rebuildList();
}

void Launcher::launch(std::size_t index) {
  if (index >= m_results.size()) {
    return;
  }
  const DesktopEntry entry = *m_results[index];
  close();
  (void)desktop_entry_launch::launchEntry(entry);
}

bool Launcher::onKeyboardEvent(const KeyboardEvent& event) {
  if (!isOpen()) {
    return false;
  }
  if (!event.pressed) {
    return true;
  }
  switch (event.sym) {
  case XKB_KEY_Escape:
    close();
    return true;
  case XKB_KEY_Return:
  case XKB_KEY_KP_Enter:
    launch(m_selected);
    return true;
  case XKB_KEY_Up:
  case XKB_KEY_ISO_Left_Tab:
    moveSelection(-1);
    return true;
  case XKB_KEY_Down:
  case XKB_KEY_Tab:
    moveSelection(1);
    return true;
  case XKB_KEY_BackSpace:
    popUtf8(m_text);
    break;
  default:
    if (event.utf32 < 0x20 || event.utf32 == 0x7F) {
      return true;
    }
    appendUtf8(m_text, event.utf32);
    break;
  }
  m_selected = 0;
  m_scroll = 0;
  filter();
  rebuildList();
  return true;
}

bool Launcher::onPointerEvent(const PointerEvent& event) {
  if (!isOpen() || event.surface != m_surface->wlSurface()) {
    return false;
  }
  const auto x = static_cast<float>(event.sx);
  const auto y = static_cast<float>(event.sy);
  switch (event.type) {
  case PointerEvent::Type::Enter:
    m_input.pointerEnter(x, y, event.serial);
    break;
  case PointerEvent::Type::Leave:
    m_input.pointerLeave();
    break;
  case PointerEvent::Type::Motion:
    m_input.pointerMotion(x, y, event.serial);
    break;
  case PointerEvent::Type::Button:
    m_input.pointerButton(x, y, event.button, event.pressed, event.serial, event.time, event.touch);
    break;
  case PointerEvent::Type::Axis:
    if (event.axisValue != 0.0) {
      moveSelection(event.axisValue > 0 ? 1 : -1);
    }
    break;
  }
  return true;
}
