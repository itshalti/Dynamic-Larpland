#include "settings/settings_window.h"

#include "compositors/hyprland.h"
#include "compositors/keybinds.h"
#include "config/config.h"
#include "core/deferred_call.h"
#include "core/input/key_modifiers.h"
#include "core/ui_phase.h"
#include "cursor-shape-v1-client-protocol.h"
#include "island/theme.h"
#include "services/notification_service.h"
#include "services/wallpaper_service.h"
#include "theme/builtin_templates.h"
#include "ui/controls/box.h"
#include "ui/controls/button.h"
#include "ui/controls/checkbox.h"
#include "ui/controls/flex.h"
#include "ui/controls/glyph.h"
#include "ui/controls/input.h"
#include "ui/controls/label.h"
#include "ui/controls/scroll_view.h"
#include "ui/controls/select.h"
#include "ui/controls/select_dropdown_popup.h"
#include "ui/controls/slider.h"
#include "ui/controls/toggle.h"
#include "wayland/layer_shell.h"
#include "wayland/wayland_connection.h"
#include "wayland/wayland_seat.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <format>
#include <xkbcommon/xkbcommon.h>

namespace {

  constexpr float kWidth = 920.0F;
  constexpr float kHeight = 640.0F;
  constexpr float kSidebarWidth = 244.0F;
  constexpr float kControlWidth = 230.0F;

  struct Section {
    const char* glyph;
    const char* title;
  };
  constexpr std::array<Section, 4> kSections = {{
      {"palette", "Look & theme"},
      {"layout-navbar", "Island & keybinds"},
      {"bell", "Notifications & calendar"},
      {"video", "Recorder, lock & power"},
  }};

  constexpr std::array<const char*, 10> kSchemes = {"m3-tonal-spot", "m3-content", "m3-fruit-salad", "m3-rainbow",
                                                    "m3-monochrome", "vibrant",    "faithful",       "soft",
                                                    "dysfunctional", "muted"};

  std::unique_ptr<Label> makeLabel(const std::string& text, float size, const Color& color, bool bold = false) {
    auto label = std::make_unique<Label>();
    label->setText(text);
    label->setFontSize(size);
    label->setColor(color);
    if (bold) {
      label->setFontWeight(FontWeight::Bold);
    }
    return label;
  }

  std::unique_ptr<Flex> makeFlex(FlexDirection direction, float gap) {
    auto flex = std::make_unique<Flex>();
    flex->setDirection(direction);
    flex->setGap(gap);
    return flex;
  }

  // A section card: title + a column of rows.
  Flex* card(Flex& page, const std::string& title) {
    auto box = makeFlex(FlexDirection::Vertical, 14.0F);
    box->setFill(theme::surfaceRaised);
    box->setRadius(18.0F);
    box->setPadding(18.0F);
    box->setAlign(FlexAlign::Stretch);
    box->addChild(makeLabel(title, theme::cardTitleFontSize, theme::textPrimary, true));
    return static_cast<Flex*>(page.addChild(std::move(box)));
  }

  // A row: title (+ description) on the left, the control on the right. Returns the control slot.
  Flex* row(Flex& parent, const std::string& title, const std::string& description = {}) {
    auto line = makeFlex(FlexDirection::Horizontal, 16.0F);
    line->setAlign(FlexAlign::Center);
    auto text = makeFlex(FlexDirection::Vertical, 2.0F);
    text->setFlexGrow(1.0F);
    text->setAlign(FlexAlign::Start);
    text->addChild(makeLabel(title, theme::bodyFontSize, theme::textPrimary));
    if (!description.empty()) {
      auto desc = makeLabel(description, theme::smallFontSize, theme::textSecondary);
      desc->setMaxWidth(kWidth - kSidebarWidth - kControlWidth - 120.0F);
      desc->setMaxLines(3);
      text->addChild(std::move(desc));
    }
    line->addChild(std::move(text));
    auto slot = makeFlex(FlexDirection::Horizontal, 8.0F);
    slot->setAlign(FlexAlign::Center);
    auto* slotPtr = static_cast<Flex*>(line->addChild(std::move(slot)));
    parent.addChild(std::move(line));
    return slotPtr;
  }

  Toggle* toggleControl(Flex& slot, bool value, std::function<void(bool)> onChange) {
    auto t = std::make_unique<Toggle>();
    t->setCheckedImmediate(value);
    t->setOnChange(std::move(onChange));
    return static_cast<Toggle*>(slot.addChild(std::move(t)));
  }

  Select* select(Flex& slot, std::vector<std::string> options, std::size_t index,
                 std::function<void(std::size_t)> onChange) {
    auto s = std::make_unique<Select>();
    s->setOptions(std::move(options));
    s->setSelectedIndexSilently(index);
    s->setSize(kControlWidth, 0.0F);
    s->setOnSelectionChanged([onChange = std::move(onChange)](std::size_t i, std::string_view) { onChange(i); });
    return static_cast<Select*>(slot.addChild(std::move(s)));
  }

  // Slider with its value shown to the left.
  void slider(Flex& slot, double min, double max, double step, double value, std::function<std::string(double)> format,
              std::function<void(double)> onCommit) {
    auto valueLabel = makeLabel(format(value), theme::smallFontSize, theme::textSecondary);
    auto* shown = static_cast<Label*>(slot.addChild(std::move(valueLabel)));
    auto s = std::make_unique<Slider>();
    s->setRange(min, max);
    s->setStep(step);
    s->setValue(value);
    s->setSize(kControlWidth - 60.0F, 0.0F);
    auto* sl = static_cast<Slider*>(slot.addChild(std::move(s)));
    sl->setOnValueChanged([shown, format](double v) { shown->setText(format(v)); });
    // Save once the drag ends, not on every step.
    sl->setOnDragEnd([sl, onCommit = std::move(onCommit)] { onCommit(sl->value()); });
  }

  Input* input(Flex& slot, const std::string& value, std::function<void(const std::string&)> onSubmit) {
    auto in = std::make_unique<Input>();
    in->setValue(value);
    in->setSize(kControlWidth, 0.0F);
    in->setOnSubmit(std::move(onSubmit));
    return static_cast<Input*>(slot.addChild(std::move(in)));
  }

  // Hyprland key name for a keysym ("Space", "A", "F5", "Return").
  std::string keyName(std::uint32_t sym) {
    std::array<char, 64> buf{};
    if (xkb_keysym_get_name(sym, buf.data(), buf.size()) <= 0) {
      return {};
    }
    std::string name = buf.data();
    if (name.size() == 1) {
      name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
    } else if (name == "space") {
      name = "Space";
    }
    return name;
  }

  bool isModifierKey(std::uint32_t sym) {
    return sym == XKB_KEY_Shift_L || sym == XKB_KEY_Shift_R || sym == XKB_KEY_Control_L || sym == XKB_KEY_Control_R
           || sym == XKB_KEY_Alt_L || sym == XKB_KEY_Alt_R || sym == XKB_KEY_Super_L || sym == XKB_KEY_Super_R
           || sym == XKB_KEY_Meta_L || sym == XKB_KEY_Meta_R || sym == XKB_KEY_ISO_Level3_Shift;
  }

  std::string actionTitle(const std::string& action) {
    if (action == "launcher") {
      return "Open launcher";
    }
    if (action == "toggle") {
      return "Expand / collapse island";
    }
    if (action == "player") {
      return "Open player";
    }
    if (action == "calendar") {
      return "Open calendar";
    }
    if (action == "wallpapers") {
      return "Open wallpapers";
    }
    if (action == "lock") {
      return "Lock screen";
    }
    if (action == "record") {
      return "Start / stop recording";
    }
    return action;
  }

} // namespace

SettingsWindow::SettingsWindow() = default;
SettingsWindow::~SettingsWindow() = default;

void SettingsWindow::initialize(WaylandConnection& wayland, RenderContext& renderContext, ConfigService& config,
                                WallpaperService& wallpapers, NotificationService& notifications) {
  m_wayland = &wayland;
  m_renderContext = &renderContext;
  m_config = &config;
  m_wallpapers = &wallpapers;
  m_notifications = &notifications;
}

bool SettingsWindow::ownsSurface(const wl_surface* surface) const noexcept {
  return m_surface != nullptr && surface == m_surface->wlSurface();
}

void SettingsWindow::toggle(const std::string& connector) {
  if (isOpen()) {
    close();
  } else {
    open(connector);
  }
}

void SettingsWindow::open(const std::string& connector) {
  if (isOpen()) {
    return;
  }
  wl_output* output = nullptr;
  for (const auto& o : m_wayland->outputs()) {
    if (output == nullptr || o.connectorName == connector) {
      output = o.output;
    }
  }
  m_recording.clear();
  m_surface = std::make_unique<LayerSurface>(
      *m_wayland,
      LayerSurfaceConfig{
          .nameSpace = "dynamic-larpland-settings",
          .layer = LayerShellLayer::Top,
          .anchor = 0,
          .width = static_cast<std::uint32_t>(kWidth),
          .height = static_cast<std::uint32_t>(kHeight),
          .exclusiveZone = -1,
          .keyboard = LayerShellKeyboard::OnDemand,
          .defaultWidth = static_cast<std::uint32_t>(kWidth),
          .defaultHeight = static_cast<std::uint32_t>(kHeight),
      }
  );
  m_surface->setRenderContext(m_renderContext);
  m_surface->setAnimationManager(&m_animations);
  m_surface->setConfigureCallback([this](std::uint32_t, std::uint32_t) { m_surface->requestLayout(); });
  m_surface->setPrepareFrameCallback([this](bool, bool) {
    UiPhaseScope layoutPhase(UiPhase::Layout);
    if (m_root == nullptr || m_rebuildPending) {
      m_rebuildPending = false;
      buildScene();
    }
    if (m_root->layoutDirty()) {
      m_root->layout(m_surface->renderTarget().renderer());
    }
  });
  if (!m_surface->initialize(output)) {
    m_surface.reset();
    return;
  }
  m_dropdown = std::make_unique<SelectDropdownPopup>(*m_wayland, *m_renderContext);
  m_dropdown->setParent(LayerShell::popupAttach(m_surface->layerSurface()), m_surface->wlSurface(), output);
}

void SettingsWindow::close() {
  if (!isOpen()) {
    return;
  }
  if (m_dropdown) {
    m_dropdown->closeSelectDropdown();
  }
  m_animations.cancelAll();
  m_input.setSceneRoot(nullptr);
  m_root.reset();
  m_pageHost = nullptr;
  m_dropdown.reset();
  m_surface.reset();
}

void SettingsWindow::requestRebuild() {
  m_rebuildPending = true;
  if (m_surface) {
    m_surface->requestLayout();
  }
}

void SettingsWindow::save() { m_config->save(); }

void SettingsWindow::buildScene() {
  m_animations.cancelAll();
  m_root = std::make_unique<Node>();
  m_root->setSize(kWidth, kHeight);

  auto frame = makeFlex(FlexDirection::Horizontal, 0.0F);
  frame->setSize(kWidth, kHeight);
  frame->setFill(theme::surface);
  frame->setBorder(theme::divider, 1.0F);
  frame->setRadius(28.0F);
  frame->setAlign(FlexAlign::Stretch);
  frame->setClipChildren(true);

  // Sidebar
  auto sidebar = makeFlex(FlexDirection::Vertical, 4.0F);
  sidebar->setMinWidth(kSidebarWidth);
  sidebar->setMaxWidth(kSidebarWidth);
  sidebar->setPadding(24.0F, 14.0F, 18.0F, 14.0F);
  sidebar->setAlign(FlexAlign::Stretch);
  auto title = makeLabel("Settings", theme::titleFontSize + 3.0F, theme::textPrimary, true);
  auto titleBox = makeFlex(FlexDirection::Horizontal, 0.0F);
  titleBox->setPadding(0.0F, 10.0F, 14.0F, 10.0F);
  titleBox->addChild(std::move(title));
  sidebar->addChild(std::move(titleBox));
  for (std::size_t i = 0; i < kSections.size(); ++i) {
    auto item = std::make_unique<InputArea>();
    item->setSize(kSidebarWidth - 28.0F, 40.0F);
    item->setCursorShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER);
    const bool selected = i == m_page;
    auto bg = std::make_unique<Box>();
    bg->setSize(kSidebarWidth - 28.0F, 40.0F);
    bg->setRadius(12.0F);
    bg->setFill(selected ? theme::surfaceRaised : Color{0, 0, 0, 0});
    auto* bgPtr = static_cast<Box*>(item->addChild(std::move(bg)));
    if (selected) {
      auto bar = std::make_unique<Box>();
      bar->setPosition(0.0F, 11.0F);
      bar->setSize(3.0F, 18.0F);
      bar->setRadius(1.5F);
      bar->setFill(theme::accentColor(*m_wallpapers));
      item->addChild(std::move(bar));
    }
    auto glyph = std::make_unique<Glyph>();
    glyph->setGlyph(kSections[i].glyph);
    glyph->setGlyphSize(17.0F);
    glyph->setColor(selected ? theme::textPrimary : theme::textSecondary);
    glyph->measure(m_surface->renderTarget().renderer());
    glyph->setPosition(14.0F, (40.0F - glyph->height()) / 2.0F);
    item->addChild(std::move(glyph));
    auto label = makeLabel(kSections[i].title, theme::bodyFontSize, selected ? theme::textPrimary : theme::textSecondary,
                           selected);
    label->measure(m_surface->renderTarget().renderer());
    label->setPosition(42.0F, (40.0F - label->height()) / 2.0F);
    item->addChild(std::move(label));
    item->setOnEnter([bgPtr, selected, this](const InputArea::PointerData&) {
      if (!selected) {
        bgPtr->setFill(theme::surfaceHover);
        m_surface->requestRedraw();
      }
    });
    item->setOnLeave([bgPtr, selected, this] {
      if (!selected) {
        bgPtr->setFill(Color{0, 0, 0, 0});
        m_surface->requestRedraw();
      }
    });
    item->setOnClick([this, i](const InputArea::PointerData&) {
      DeferredCall::callLater([this, i] { showPage(i); });
    });
    sidebar->addChild(std::move(item));
  }
  frame->addChild(std::move(sidebar));

  auto line = std::make_unique<Box>();
  line->setSize(1.0F, kHeight);
  line->setFill(theme::divider);
  frame->addChild(std::move(line));

  // Page area
  auto host = makeFlex(FlexDirection::Vertical, 0.0F);
  host->setFlexGrow(1.0F);
  host->setAlign(FlexAlign::Stretch);
  m_pageHost = frame->addChild(std::move(host));
  m_pageHost->addChild(buildPage(m_page));
  m_root->addChild(std::move(frame));

  // Close button (top right, above the page).
  auto close = std::make_unique<InputArea>();
  close->setPosition(kWidth - 52.0F, 16.0F);
  close->setSize(34.0F, 34.0F);
  close->setCursorShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER);
  close->setZIndex(10);
  auto x = std::make_unique<Glyph>();
  x->setGlyph("x");
  x->setGlyphSize(18.0F);
  x->setColor(theme::textSecondary);
  x->measure(m_surface->renderTarget().renderer());
  x->setPosition((34.0F - x->width()) / 2.0F, (34.0F - x->height()) / 2.0F);
  close->addChild(std::move(x));
  close->setOnClick([this](const InputArea::PointerData&) { DeferredCall::callLater([this] { this->close(); }); });
  m_root->addChild(std::move(close));

  m_root->setPopupContext(m_dropdown.get());
  m_surface->setSceneRoot(m_root.get());
  m_input.setSceneRoot(m_root.get());
  m_input.setCursorShapeCallback([this](std::uint32_t serial, std::uint32_t shape) {
    m_wayland->setCursorShape(serial, shape);
  });
}

void SettingsWindow::showPage(std::size_t index) {
  m_page = index;
  m_recording.clear();
  requestRebuild();
}

std::unique_ptr<Node> SettingsWindow::buildPage(std::size_t index) {
  auto scroll = std::make_unique<ScrollView>();
  scroll->setFlexGrow(1.0F);
  scroll->setSize(kWidth - kSidebarWidth - 1.0F, kHeight);
  Flex* page = scroll->content();
  page->setDirection(FlexDirection::Vertical);
  page->setGap(14.0F);
  page->setPadding(26.0F, 30.0F, 30.0F, 28.0F);
  page->setAlign(FlexAlign::Stretch);
  page->addChild(makeLabel(kSections[index].title, theme::hugeFontSize - 6.0F, theme::textPrimary, true));
  switch (index) {
  case 0:
    buildLookPage(*page);
    break;
  case 1:
    buildIslandPage(*page);
    break;
  case 2:
    buildNotificationsPage(*page);
    break;
  default:
    buildSystemPage(*page);
    break;
  }
  return scroll;
}

void SettingsWindow::buildLookPage(Flex& page) {
  auto& cfg = m_config->mutableConfig();
  // Re-run palette + templates for the current wallpaper after a theme option changed.
  auto reapply = [this] {
    if (!m_wallpapers->current().empty()) {
      m_wallpapers->set(m_wallpapers->current());
    }
  };

  Flex* appearance = card(page, "Appearance");
  const auto schemeIt = std::ranges::find(kSchemes, cfg.theme.scheme);
  select(*row(*appearance, "Color scheme", "How the palette is built from the wallpaper."),
         std::vector<std::string>(kSchemes.begin(), kSchemes.end()),
         schemeIt != kSchemes.end() ? static_cast<std::size_t>(schemeIt - kSchemes.begin()) : 0,
         [this, reapply](std::size_t i) {
           m_config->mutableConfig().theme.scheme = kSchemes[i];
           save();
           reapply();
         });
  select(*row(*appearance, "Mode", "Palette variant used for the island accent and the app templates."),
         {"Dark", "Light"}, cfg.theme.mode == "light" ? 1 : 0, [this, reapply](std::size_t i) {
           m_config->mutableConfig().theme.mode = i == 1 ? "light" : "dark";
           save();
           reapply();
         });

  Flex* wallpaper = card(page, "Wallpaper");
  input(*row(*wallpaper, "Wallpaper folder", "Images shown in the island's wallpaper picker."), cfg.wallpaper.dir,
        [this](const std::string& value) {
          if (!value.empty()) {
            m_config->mutableConfig().wallpaper.dir = value;
            save();
          }
        });
  const std::string current = m_wallpapers->current().empty() ? std::string("None") : m_wallpapers->current();
  auto currentLabel = makeLabel(current, theme::smallFontSize, theme::textSecondary);
  currentLabel->setMaxWidth(kControlWidth + 60.0F);
  row(*wallpaper, "Current wallpaper")->addChild(std::move(currentLabel));

  Flex* templates = card(page, "App templates");
  toggleControl(*row(*templates, "Enable built-in templates", "Recolor the selected apps on every wallpaper change."),
         cfg.theme.builtinTemplates, [this, reapply](bool on) {
           m_config->mutableConfig().theme.builtinTemplates = on;
           save();
           reapply();
         });
  // Grid of template tiles (checkbox + name + category), like Noctalia's.
  auto grid = makeFlex(FlexDirection::Horizontal, 10.0F);
  grid->setWrap(true);
  for (const auto& info : noctalia::theme::loadBuiltinTemplateInfo()) {
    const bool enabled = std::ranges::contains(cfg.theme.templates, info.id);
    auto tile = makeFlex(FlexDirection::Horizontal, 10.0F);
    tile->setAlign(FlexAlign::Center);
    tile->setPadding(10.0F, 12.0F, 10.0F, 12.0F);
    tile->setMinWidth(184.0F);
    tile->setMaxWidth(184.0F);
    tile->setRadius(12.0F);
    tile->setFill(enabled ? theme::surfaceHover : theme::surface);
    tile->setBorder(theme::divider, 1.0F);
    auto check = std::make_unique<Checkbox>();
    check->setChecked(enabled);
    check->setOnChange([this, id = info.id](bool on) {
      auto& list = m_config->mutableConfig().theme.templates;
      std::erase(list, id);
      if (on) {
        list.push_back(id);
      }
      save();
      requestRebuild();
    });
    tile->addChild(std::move(check));
    auto text = makeFlex(FlexDirection::Vertical, 1.0F);
    text->setAlign(FlexAlign::Start);
    text->addChild(makeLabel(info.name, theme::bodyFontSize - 1.0F, theme::textPrimary));
    text->addChild(makeLabel(info.category, theme::tinyFontSize, theme::textSecondary));
    tile->addChild(std::move(text));
    grid->addChild(std::move(tile));
  }
  templates->addChild(std::move(grid));
  templates->addChild(makeLabel("Changes apply on the next wallpaper pick (or now, when a wallpaper is set).",
                                theme::smallFontSize, theme::textTertiary));
  if (!cfg.wallpaper.current.empty()) {
    auto apply = std::make_unique<Button>();
    apply->setText("Apply now");
    apply->setVariant(ButtonVariant::Secondary);
    apply->setOnClick([reapply] { reapply(); });
    row(*templates, "Recolor apps with the current wallpaper")->addChild(std::move(apply));
  }
}

void SettingsWindow::buildIslandPage(Flex& page) {
  auto& cfg = m_config->mutableConfig();
  Flex* behaviour = card(page, "Behaviour");
  toggleControl(*row(*behaviour, "Expand on hover", "Open the island when the pointer rests on it."), cfg.island.hoverExpand,
         [this](bool on) {
           m_config->mutableConfig().island.hoverExpand = on;
           save();
         });
  slider(*row(*behaviour, "Hover delay"), 0, 1000, 25, cfg.island.hoverDelayMs,
         [](double v) { return std::format("{} ms", static_cast<int>(v)); },
         [this](double v) {
           m_config->mutableConfig().island.hoverDelayMs = static_cast<int>(v);
           save();
         });
  constexpr std::array<std::pair<const char*, const char*>, 3> kFormats = {{
      {"24-hour (23:59)", "%H:%M"},
      {"24-hour with seconds", "%H:%M:%S"},
      {"12-hour (11:59 PM)", "%I:%M %p"},
  }};
  std::size_t formatIndex = 0;
  for (std::size_t i = 0; i < kFormats.size(); ++i) {
    if (cfg.island.timeFormat == kFormats[i].second) {
      formatIndex = i;
    }
  }
  select(*row(*behaviour, "Clock format"), {kFormats[0].first, kFormats[1].first, kFormats[2].first}, formatIndex,
         [this, kFormats](std::size_t i) {
           m_config->mutableConfig().island.timeFormat = kFormats[i].second;
           save();
         });

  Flex* binds = card(page, "Keybinds");
  for (const char* action : keybinds::kActions) {
    const std::string keys = cfg.keybinds[action];
    const bool recording = m_recording == action;
    auto chip = std::make_unique<Button>();
    chip->setText(recording ? "Press keys..." : keys.empty() ? "Not set" : keys);
    chip->setVariant(recording ? ButtonVariant::Primary : ButtonVariant::Secondary);
    chip->setOnClick([this, name = std::string(action)] {
      m_recording = m_recording == name ? std::string() : name;
      requestRebuild();
    });
    row(*binds, actionTitle(action))->addChild(std::move(chip));
  }
  const std::string hint =
      HyprlandService::available()
          ? "Click a key, press the new combo (Esc cancels, Backspace clears). Saved to " + keybinds::bindsPath()
                + " - load it from your Hyprland config with: " + keybinds::includeLine()
          : "Bind these in your compositor to: " + keybinds::ipcCommand("<action>");
  auto hintLabel = makeLabel(hint, theme::smallFontSize, theme::textTertiary);
  hintLabel->setMaxWidth(kWidth - kSidebarWidth - 110.0F);
  hintLabel->setMaxLines(4);
  binds->addChild(std::move(hintLabel));
}

void SettingsWindow::buildNotificationsPage(Flex& page) {
  auto& cfg = m_config->mutableConfig();
  Flex* notifications = card(page, "Notifications");
  slider(*row(*notifications, "History size", "How many notifications are kept."), 10, 500, 10,
         cfg.notifications.historyLimit, [](double v) { return std::format("{}", static_cast<int>(v)); },
         [this](double v) {
           m_config->mutableConfig().notifications.historyLimit = static_cast<int>(v);
           save();
         });
  slider(*row(*notifications, "Popup duration", "How long a new notification shows in the pill."), 1000, 15000,
         500, cfg.notifications.popupTimeoutMs, [](double v) { return std::format("{:.1f} s", v / 1000.0); },
         [this](double v) {
           m_config->mutableConfig().notifications.popupTimeoutMs = static_cast<int>(v);
           save();
         });
  auto clear = std::make_unique<Button>();
  clear->setText("Clear history");
  clear->setVariant(ButtonVariant::Destructive);
  clear->setOnClick([this] { m_notifications->clear(); });
  row(*notifications, "Notification history",
      std::format("{} stored", m_notifications->history().size()))->addChild(std::move(clear));

  Flex* calendar = card(page, "Calendar");
  select(*row(*calendar, "First day of the week"), {"Sunday", "Monday"}, cfg.calendar.firstDayOfWeek == 1 ? 1 : 0,
         [this](std::size_t i) {
           m_config->mutableConfig().calendar.firstDayOfWeek = static_cast<int>(i);
           save();
         });
}

void SettingsWindow::buildSystemPage(Flex& page) {
  auto& cfg = m_config->mutableConfig();
  Flex* recorder = card(page, "Screen recorder");
  input(*row(*recorder, "Recordings folder"), cfg.recorder.dir, [this](const std::string& value) {
    if (!value.empty()) {
      m_config->mutableConfig().recorder.dir = value;
      save();
    }
  });
  constexpr std::array<int, 6> kFps = {30, 60, 120, 144, 165, 240};
  const auto fpsIt = std::ranges::find(kFps, cfg.recorder.fps);
  select(*row(*recorder, "Frame rate"), {"30", "60", "120", "144", "165", "240"},
         fpsIt != kFps.end() ? static_cast<std::size_t>(fpsIt - kFps.begin()) : 1, [this, kFps](std::size_t i) {
           m_config->mutableConfig().recorder.fps = kFps[i];
           save();
         });
  constexpr std::array<std::pair<const char*, const char*>, 4> kAudio = {{
      {"Desktop audio", "default_output"},
      {"Microphone", "default_input"},
      {"Desktop + microphone", "default_output|default_input"},
      {"No audio", ""},
  }};
  std::size_t audioIndex = 0;
  for (std::size_t i = 0; i < kAudio.size(); ++i) {
    if (cfg.recorder.audio == kAudio[i].second) {
      audioIndex = i;
    }
  }
  select(*row(*recorder, "Audio"), {kAudio[0].first, kAudio[1].first, kAudio[2].first, kAudio[3].first}, audioIndex,
         [this, kAudio](std::size_t i) {
           m_config->mutableConfig().recorder.audio = kAudio[i].second;
           save();
         });

  Flex* lock = card(page, "Lock & power");
  lock->addChild(makeLabel("The lock screen checks your password through PAM (the \"login\" stack).",
                           theme::smallFontSize, theme::textSecondary));
  lock->addChild(makeLabel("Shut down, reboot and reboot to UEFI on the power page need a second click to confirm.",
                           theme::smallFontSize, theme::textSecondary));
}

bool SettingsWindow::onPointerEvent(const PointerEvent& event) {
  if (!isOpen()) {
    return false;
  }
  if (m_dropdown && m_dropdown->onPointerEvent(event)) {
    return true;
  }
  if (event.surface != m_surface->wlSurface()) {
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
    m_input.pointerAxis(x, y, event.axis, event.axisSource, event.axisValue, event.axisDiscrete, event.axisValue120,
                        event.axisLines, event.axisGestureSerial);
    break;
  }
  m_surface->requestRedraw();
  return true;
}

bool SettingsWindow::onKeyboardEvent(const KeyboardEvent& event) {
  if (!isOpen()) {
    return false;
  }
  if (m_dropdown && m_dropdown->isSelectDropdownOpen()) {
    m_dropdown->onKeyboardEvent(event);
    return true;
  }
  if (!m_recording.empty()) {
    if (!event.pressed || isModifierKey(event.sym)) {
      return true;
    }
    auto& binds = m_config->mutableConfig().keybinds;
    if (event.sym == XKB_KEY_Escape) {
      m_recording.clear();
    } else if (event.sym == XKB_KEY_BackSpace || event.sym == XKB_KEY_Delete) {
      binds[m_recording] = "";
      m_recording.clear();
      save();
    } else if (const std::string key = keyName(event.sym); !key.empty()) {
      std::string combo;
      for (const auto& [bit, name] : {std::pair{KeyMod::Super, "SUPER"}, std::pair{KeyMod::Ctrl, "CTRL"},
                                      std::pair{KeyMod::Alt, "ALT"}, std::pair{KeyMod::Shift, "SHIFT"}}) {
        if ((event.modifiers & bit) != 0) {
          combo += std::string(name) + " + ";
        }
      }
      binds[m_recording] = combo + key;
      m_recording.clear();
      save();
    }
    requestRebuild();
    return true;
  }
  if (event.pressed && event.sym == XKB_KEY_Escape && m_input.focusedArea() == nullptr) {
    close();
    return true;
  }
  m_input.keyEvent(event.sym, event.utf32, event.modifiers, event.pressed, event.preedit);
  m_surface->requestRedraw();
  return true;
}
