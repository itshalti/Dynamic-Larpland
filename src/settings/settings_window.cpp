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
#include "settings/settings_registry.h"
#include "system/day_night_schedule.h"
#include "theme/builtin_templates.h"
#include "theme/community_templates.h"
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

  constexpr float kWidth = 960.0F;
  constexpr float kHeight = 660.0F;
  constexpr float kSidebarWidth = 220.0F;
  constexpr float kControlWidth = 230.0F;
  constexpr float kHeaderHeight = 64.0F;

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

  std::string lower(std::string s) {
    std::ranges::transform(s, s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
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
    static const std::array<std::pair<const char*, const char*>, 7> kTitles = {{
        {"launcher", "Open launcher"},
        {"toggle", "Expand / collapse island"},
        {"player", "Open player"},
        {"calendar", "Open calendar"},
        {"wallpapers", "Open wallpapers"},
        {"lock", "Lock screen"},
        {"record", "Start / stop recording"},
    }};
    for (const auto& [id, title] : kTitles) {
      if (action == id) {
        return title;
      }
    }
    return action;
  }

  // Current value at a path as the string a Select option uses.
  std::string optionValue(const toml::node_view<const toml::node>& node) {
    if (const auto i = node.value<std::int64_t>()) {
      return std::to_string(*i);
    }
    return node.value_or(std::string());
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

void SettingsWindow::openSection(const std::string& connector, const std::string& section) {
  const auto& list = settings::sections();
  const auto it = std::ranges::find(list, section, &settings::Section::id);
  if (it != list.end()) {
    m_section = static_cast<std::size_t>(it - list.begin());
  }
  if (isOpen()) {
    requestRefresh();
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
  m_query.clear();
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
    if (m_root == nullptr) {
      buildScene();
    } else if (m_refreshPending) {
      refreshContent();
    }
    m_refreshPending = false;
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
  m_sidebar = nullptr;
  m_pageHost = nullptr;
  m_search = nullptr;
  m_dropdown.reset();
  m_surface.reset();
}

void SettingsWindow::requestRefresh() {
  m_refreshPending = true;
  if (m_surface) {
    m_surface->requestLayout();
  }
}

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

  auto sidebar = makeFlex(FlexDirection::Vertical, 2.0F);
  sidebar->setMinWidth(kSidebarWidth);
  sidebar->setMaxWidth(kSidebarWidth);
  sidebar->setPadding(22.0F, 12.0F, 18.0F, 12.0F);
  sidebar->setAlign(FlexAlign::Stretch);
  m_sidebar = static_cast<Flex*>(frame->addChild(std::move(sidebar)));

  auto line = std::make_unique<Box>();
  line->setSize(1.0F, kHeight);
  line->setFill(theme::divider);
  frame->addChild(std::move(line));

  auto column = makeFlex(FlexDirection::Vertical, 0.0F);
  column->setFlexGrow(1.0F);
  column->setAlign(FlexAlign::Stretch);
  buildHeader(*column);
  auto host = makeFlex(FlexDirection::Vertical, 0.0F);
  host->setFlexGrow(1.0F);
  host->setAlign(FlexAlign::Stretch);
  m_pageHost = static_cast<Flex*>(column->addChild(std::move(host)));
  frame->addChild(std::move(column));
  m_root->addChild(std::move(frame));

  m_surface->setSceneRoot(m_root.get());
  m_input.setSceneRoot(m_root.get());
  m_input.setCursorShapeCallback([this](std::uint32_t serial, std::uint32_t shape) {
    m_wayland->setCursorShape(serial, shape);
  });
  refreshContent();
}

void SettingsWindow::buildHeader(Flex& column) {
  // Search | Advanced | Overridden | Reset page | close
  auto header = makeFlex(FlexDirection::Horizontal, 10.0F);
  header->setAlign(FlexAlign::Center);
  header->setPadding(16.0F, 16.0F, 8.0F, 24.0F);
  header->setMinHeight(kHeaderHeight);

  auto search = std::make_unique<Input>();
  search->setPlaceholder("Search settings...");
  search->setSize(250.0F, 0.0F);
  search->setOnChange([this](const std::string& text) {
    m_query = text;
    requestRefresh();
  });
  m_search = static_cast<Input*>(header->addChild(std::move(search)));

  auto spacer = std::make_unique<Node>();
  spacer->setFlexGrow(1.0F);
  header->addChild(std::move(spacer));

  auto filter = [&](const char* label, bool value, std::function<void(bool)> onChange) {
    header->addChild(makeLabel(label, theme::smallFontSize, theme::textSecondary));
    auto toggle = std::make_unique<Toggle>();
    toggle->setCheckedImmediate(value);
    toggle->setOnChange(std::move(onChange));
    header->addChild(std::move(toggle));
  };
  filter("Advanced", m_showAdvanced, [this](bool on) {
    m_showAdvanced = on;
    requestRefresh();
  });
  filter("Overridden", m_onlyOverridden, [this](bool on) {
    m_onlyOverridden = on;
    requestRefresh();
  });

  auto reset = std::make_unique<Button>();
  reset->setText("Reset page");
  reset->setVariant(ButtonVariant::Ghost);
  reset->setOnClick([this] { DeferredCall::callLater([this] { resetPage(); }); });
  header->addChild(std::move(reset));

  auto close = std::make_unique<Button>();
  close->setGlyph("x");
  close->setVariant(ButtonVariant::Ghost);
  close->setOnClick([this] { DeferredCall::callLater([this] { this->close(); }); });
  header->addChild(std::move(close));

  column.addChild(std::move(header));
}

void SettingsWindow::refreshContent() {
  m_animations.cancelAll();
  buildSidebar();
  while (!m_pageHost->children().empty()) {
    m_pageHost->removeChild(m_pageHost->children().back().get());
  }
  m_pageHost->addChild(buildPage());
  m_root->setPopupContext(m_dropdown.get());
}

void SettingsWindow::buildSidebar() {
  while (!m_sidebar->children().empty()) {
    m_sidebar->removeChild(m_sidebar->children().back().get());
  }
  Renderer& renderer = m_surface->renderTarget().renderer();
  auto titleBox = makeFlex(FlexDirection::Horizontal, 0.0F);
  titleBox->setPadding(0.0F, 10.0F, 14.0F, 10.0F);
  titleBox->addChild(makeLabel("Settings", theme::titleFontSize + 3.0F, theme::textPrimary, true));
  m_sidebar->addChild(std::move(titleBox));

  const auto& sections = settings::sections();
  constexpr float kItemH = 36.0F;
  const float itemW = kSidebarWidth - 24.0F;
  for (std::size_t i = 0; i < sections.size(); ++i) {
    // While searching no section is selected: results come from all of them.
    const bool selected = m_query.empty() && i == m_section;
    auto item = std::make_unique<InputArea>();
    item->setSize(itemW, kItemH);
    item->setCursorShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER);
    auto bg = std::make_unique<Box>();
    bg->setSize(itemW, kItemH);
    bg->setRadius(11.0F);
    bg->setFill(selected ? theme::surfaceRaised : Color{0, 0, 0, 0});
    auto* bgPtr = static_cast<Box*>(item->addChild(std::move(bg)));
    if (selected) {
      auto bar = std::make_unique<Box>();
      bar->setPosition(0.0F, 10.0F);
      bar->setSize(3.0F, kItemH - 20.0F);
      bar->setRadius(1.5F);
      bar->setFill(theme::accentColor(*m_wallpapers));
      item->addChild(std::move(bar));
    }
    auto glyph = std::make_unique<Glyph>();
    glyph->setGlyph(sections[i].glyph);
    glyph->setGlyphSize(16.0F);
    glyph->setColor(selected ? theme::textPrimary : theme::textSecondary);
    glyph->measure(renderer);
    glyph->setPosition(14.0F, (kItemH - glyph->height()) / 2.0F);
    item->addChild(std::move(glyph));
    auto label = makeLabel(sections[i].title, theme::bodyFontSize, selected ? theme::textPrimary : theme::textSecondary,
                           selected);
    label->measure(renderer);
    label->setPosition(40.0F, (kItemH - label->height()) / 2.0F);
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
      m_section = i;
      m_recording.clear();
      m_query.clear();
      if (m_search != nullptr) {
        m_search->setValue("");
      }
      requestRefresh();
    });
    m_sidebar->addChild(std::move(item));
  }
}

bool SettingsWindow::visible(const settings::Setting& setting) const {
  if (setting.advanced && !m_showAdvanced) {
    return false;
  }
  if (m_onlyOverridden && (setting.path.empty() || !m_config->overridden(setting.path))) {
    return false;
  }
  if (m_query.empty()) {
    return setting.section == settings::sections()[m_section].id;
  }
  const std::string q = lower(m_query);
  return lower(setting.title).contains(q) || lower(setting.description).contains(q)
         || lower(setting.group).contains(q) || lower(setting.section).contains(q);
}

std::unique_ptr<Node> SettingsWindow::buildPage() {
  auto scroll = std::make_unique<ScrollView>();
  scroll->setFlexGrow(1.0F);
  scroll->setSize(kWidth - kSidebarWidth - 1.0F, kHeight - kHeaderHeight);
  Flex* page = scroll->content();
  page->setDirection(FlexDirection::Vertical);
  page->setGap(14.0F);
  page->setPadding(4.0F, 30.0F, 30.0F, 24.0F);
  page->setAlign(FlexAlign::Stretch);

  const auto& sections = settings::sections();
  page->addChild(makeLabel(m_query.empty() ? sections[m_section].title : "Search results",
                           theme::hugeFontSize - 6.0F, theme::textPrimary, true));

  // Rows grouped into cards, in registry order; search results are titled "Section · Group".
  std::string currentCard;
  Flex* cardNode = nullptr;
  for (const auto& setting : settings::registry()) {
    if (!visible(setting)) {
      continue;
    }
    std::string cardTitle = setting.group;
    if (!m_query.empty()) {
      const auto it = std::ranges::find(sections, setting.section, &settings::Section::id);
      cardTitle = (it != sections.end() ? it->title : setting.section) + " · " + setting.group;
    }
    if (cardNode == nullptr || cardTitle != currentCard) {
      currentCard = cardTitle;
      cardNode = card(*page, cardTitle);
    }
    buildRow(*cardNode, setting);
  }
  if (cardNode == nullptr) {
    page->addChild(makeLabel(m_onlyOverridden ? "Nothing here differs from the defaults." : "Nothing matches.",
                             theme::bodyFontSize, theme::textSecondary));
  }
  return scroll;
}

void SettingsWindow::afterChange(const std::string& path) {
  // Theme options re-run the palette + templates for the current wallpaper.
  if (path.starts_with("theme.") && path != "theme.auto_mode" && !m_wallpapers->current().empty()) {
    m_wallpapers->set(m_wallpapers->current());
  }
  requestRefresh();
}

void SettingsWindow::resetPage() {
  bool theme = false;
  for (const auto& setting : settings::registry()) {
    if (!setting.path.empty() && visible(setting) && m_config->overridden(setting.path)) {
      m_config->reset(setting.path);
      theme = theme || setting.path.starts_with("theme.");
    }
  }
  if (theme && !m_wallpapers->current().empty()) {
    m_wallpapers->set(m_wallpapers->current());
  }
  requestRefresh();
}

void SettingsWindow::buildRow(Flex& cardNode, const settings::Setting& setting) {
  if (setting.kind == settings::Kind::Custom) {
    buildCustom(cardNode, setting);
    return;
  }
  auto line = makeFlex(FlexDirection::Horizontal, 12.0F);
  line->setAlign(FlexAlign::Center);
  auto text = makeFlex(FlexDirection::Vertical, 2.0F);
  text->setFlexGrow(1.0F);
  text->setAlign(FlexAlign::Start);
  text->addChild(makeLabel(setting.title, theme::bodyFontSize, theme::textPrimary));
  if (!setting.description.empty()) {
    auto desc = makeLabel(setting.description, theme::smallFontSize, theme::textSecondary);
    desc->setMaxWidth(kWidth - kSidebarWidth - kControlWidth - 140.0F);
    desc->setMaxLines(3);
    text->addChild(std::move(desc));
  }
  line->addChild(std::move(text));

  // Changed from the default: a reset button before the control.
  const std::string path = setting.path;
  if (m_config->overridden(path)) {
    auto reset = std::make_unique<Button>();
    reset->setGlyph("refresh");
    reset->setVariant(ButtonVariant::Ghost);
    reset->setOnClick([this, path] {
      m_config->reset(path);
      afterChange(path);
    });
    line->addChild(std::move(reset));
  }

  const toml::table table = m_config->asTable();
  const auto node = table.at_path(path);
  switch (setting.kind) {
  case settings::Kind::Toggle: {
    auto toggle = std::make_unique<Toggle>();
    toggle->setCheckedImmediate(node.value_or(false));
    toggle->setOnChange([this, path](bool on) {
      m_config->set(path, toml::value<bool>(on));
      afterChange(path);
    });
    line->addChild(std::move(toggle));
    break;
  }
  case settings::Kind::Select: {
    auto select = std::make_unique<Select>();
    std::vector<std::string> labels;
    std::size_t index = 0;
    const std::string current = optionValue(node);
    for (std::size_t i = 0; i < setting.options.size(); ++i) {
      labels.push_back(setting.options[i].label);
      if (setting.options[i].value == current) {
        index = i;
      }
    }
    select->setOptions(std::move(labels));
    select->setSelectedIndexSilently(index);
    select->setSize(kControlWidth, 0.0F);
    select->setOnSelectionChanged([this, path, options = setting.options,
                                   integer = setting.integerValue](std::size_t i, std::string_view) {
      if (integer) {
        m_config->set(path, toml::value<std::int64_t>(std::stoll(options[i].value)));
      } else {
        m_config->set(path, toml::value<std::string>(options[i].value));
      }
      afterChange(path);
    });
    line->addChild(std::move(select));
    break;
  }
  case settings::Kind::Slider: {
    const double value = node.value_or(0.0);
    // Whole units show as integers ("150 ms"), scaled ones as decimals ("4.0 s").
    auto format = [unit = setting.unitFormat, scale = setting.displayScale](double v) {
      if (scale == 1.0) {
        const int whole = static_cast<int>(v);
        return std::vformat(unit, std::make_format_args(whole));
      }
      const double scaled = v / scale;
      return std::vformat(unit, std::make_format_args(scaled));
    };
    auto* shown = static_cast<Label*>(line->addChild(makeLabel(format(value), theme::smallFontSize,
                                                                theme::textSecondary)));
    auto slider = std::make_unique<Slider>();
    slider->setRange(setting.min, setting.max);
    slider->setStep(setting.step);
    slider->setValue(value);
    slider->setSize(kControlWidth - 60.0F, 0.0F);
    auto* sl = static_cast<Slider*>(line->addChild(std::move(slider)));
    sl->setOnValueChanged([shown, format](double v) { shown->setText(format(v)); });
    // Save once the drag ends, not on every step.
    sl->setOnDragEnd([this, sl, path] {
      m_config->set(path, toml::value<std::int64_t>(static_cast<std::int64_t>(sl->value())));
      afterChange(path);
    });
    break;
  }
  case settings::Kind::Text: {
    auto input = std::make_unique<Input>();
    if (setting.numberValue) {
      const auto number = node.value<double>();
      input->setValue(number ? std::format("{}", *number) : std::string());
    } else {
      input->setValue(node.value_or(std::string()));
    }
    input->setSize(kControlWidth, 0.0F);
    auto* inputPtr = input.get();
    input->setOnSubmit([this, path, inputPtr, number = setting.numberValue](const std::string& value) {
      if (number) {
        // "" clears the value; anything else must parse as a number.
        if (value.empty()) {
          m_config->reset(path);
        } else {
          try {
            std::size_t used = 0;
            const double parsed = std::stod(value, &used);
            if (used != value.size()) {
              throw std::invalid_argument(value);
            }
            m_config->set(path, toml::value<double>(parsed));
          } catch (const std::exception&) {
            inputPtr->setInvalid(true);
            return;
          }
        }
        afterChange(path);
      } else if (!value.empty()) {
        m_config->set(path, toml::value<std::string>(value));
        afterChange(path);
      }
    });
    line->addChild(std::move(input));
    break;
  }
  case settings::Kind::Custom:
    break;
  }
  cardNode.addChild(std::move(line));
}

void SettingsWindow::buildCustom(Flex& cardNode, const settings::Setting& setting) {
  auto& cfg = m_config->mutableConfig();
  auto labelRow = [&](const std::string& title, std::unique_ptr<Node> control, const std::string& description = {}) {
    auto line = makeFlex(FlexDirection::Horizontal, 12.0F);
    line->setAlign(FlexAlign::Center);
    auto text = makeFlex(FlexDirection::Vertical, 2.0F);
    text->setFlexGrow(1.0F);
    text->setAlign(FlexAlign::Start);
    text->addChild(makeLabel(title, theme::bodyFontSize, theme::textPrimary));
    if (!description.empty()) {
      text->addChild(makeLabel(description, theme::smallFontSize, theme::textSecondary));
    }
    line->addChild(std::move(text));
    if (control) {
      line->addChild(std::move(control));
    }
    cardNode.addChild(std::move(line));
  };

  if (setting.id == "current-wallpaper") {
    auto current = makeLabel(m_wallpapers->current().empty() ? std::string("None") : m_wallpapers->current(),
                             theme::smallFontSize, theme::textSecondary);
    current->setMaxWidth(kControlWidth + 80.0F);
    labelRow(setting.title, std::move(current));
  } else if (setting.id == "template-grid" || setting.id == "community-grid") {
    // Tiles (checkbox + name + category), like Noctalia's Built-In / Community tabs.
    const bool community = setting.id == "community-grid";
    std::vector<std::pair<std::string, std::pair<std::string, std::string>>> entries;
    if (community) {
      for (const auto& t : noctalia::theme::CommunityTemplateService::availableTemplates()) {
        entries.push_back({t.id, {t.displayName.empty() ? t.id : t.displayName, t.category}});
      }
    } else {
      for (const auto& info : noctalia::theme::loadBuiltinTemplateInfo()) {
        entries.push_back({info.id, {info.name, info.category}});
      }
    }
    const auto& selected = community ? cfg.theme.community : cfg.theme.templates;
    if (entries.empty()) {
      cardNode.addChild(makeLabel(cfg.theme.communityTemplates ? "Downloading the catalog..."
                                                               : "Enable community templates to see the catalog.",
                                  theme::smallFontSize, theme::textTertiary));
    }
    auto grid = makeFlex(FlexDirection::Horizontal, 10.0F);
    grid->setWrap(true);
    for (const auto& [id, meta] : entries) {
      const bool enabled = std::ranges::contains(selected, id);
      auto tile = makeFlex(FlexDirection::Horizontal, 10.0F);
      tile->setAlign(FlexAlign::Center);
      tile->setPadding(10.0F, 12.0F, 10.0F, 12.0F);
      tile->setMinWidth(196.0F);
      tile->setMaxWidth(196.0F);
      tile->setRadius(12.0F);
      tile->setFill(enabled ? theme::surfaceHover : theme::surface);
      tile->setBorder(theme::divider, 1.0F);
      auto check = std::make_unique<Checkbox>();
      check->setChecked(enabled);
      check->setOnChange([this, id = id, community](bool on) {
        auto& theme = m_config->mutableConfig().theme;
        auto& list = community ? theme.community : theme.templates;
        std::erase(list, id);
        if (on) {
          list.push_back(id);
        }
        m_config->save();
        requestRefresh();
      });
      tile->addChild(std::move(check));
      auto text = makeFlex(FlexDirection::Vertical, 1.0F);
      text->setAlign(FlexAlign::Start);
      auto name = makeLabel(meta.first, theme::bodyFontSize - 1.0F, theme::textPrimary);
      name->setMaxWidth(140.0F);
      text->addChild(std::move(name));
      text->addChild(makeLabel(meta.second, theme::tinyFontSize, theme::textSecondary));
      tile->addChild(std::move(text));
      grid->addChild(std::move(tile));
    }
    cardNode.addChild(std::move(grid));
    if (!community && !cfg.wallpaper.current.empty()) {
      auto apply = std::make_unique<Button>();
      apply->setText("Apply now");
      apply->setVariant(ButtonVariant::Secondary);
      apply->setOnClick([this] { m_wallpapers->set(m_wallpapers->current()); });
      labelRow("Recolor apps with the current wallpaper", std::move(apply));
    }
  } else if (setting.id == "keybinds") {
    for (const char* action : keybinds::kActions) {
      const std::string keys = cfg.keybinds[action];
      const bool recording = m_recording == action;
      auto chip = std::make_unique<Button>();
      chip->setText(recording ? "Press keys..." : keys.empty() ? "Not set" : keys);
      chip->setVariant(recording ? ButtonVariant::Primary : ButtonVariant::Secondary);
      chip->setOnClick([this, name = std::string(action)] {
        m_recording = m_recording == name ? std::string() : name;
        requestRefresh();
      });
      labelRow(actionTitle(action), std::move(chip));
    }
    const std::string hint =
        HyprlandService::available()
            ? "Click a key, press the new combo (Esc cancels, Backspace clears). Saved to " + keybinds::bindsPath()
                  + " - load it from your Hyprland config with: " + keybinds::includeLine()
            : "Bind these in your compositor to: " + keybinds::ipcCommand("<action>");
    auto hintLabel = makeLabel(hint, theme::smallFontSize, theme::textTertiary);
    hintLabel->setMaxWidth(kWidth - kSidebarWidth - 110.0F);
    hintLabel->setMaxLines(4);
    cardNode.addChild(std::move(hintLabel));
  } else if (setting.id == "clear-history") {
    auto clear = std::make_unique<Button>();
    clear->setText("Clear history");
    clear->setVariant(ButtonVariant::Destructive);
    clear->setOnClick([this] {
      m_notifications->clear();
      requestRefresh();
    });
    labelRow(setting.title, std::move(clear), std::format("{} stored", m_notifications->history().size()));
  } else if (setting.id == "notification-rules") {
    auto& rules = cfg.notifications.rules;
    for (std::size_t i = 0; i < rules.size(); ++i) {
      auto controls = makeFlex(FlexDirection::Horizontal, 8.0F);
      controls->setAlign(FlexAlign::Center);
      auto action = std::make_unique<Select>();
      action->setOptions({"Mute", "Block"});
      action->setSelectedIndexSilently(rules[i].action == "block" ? 1 : 0);
      action->setSize(140.0F, 0.0F);
      action->setOnSelectionChanged([this, i](std::size_t index, std::string_view) {
        auto& list = m_config->mutableConfig().notifications.rules;
        if (i < list.size()) {
          list[i].action = index == 1 ? "block" : "mute";
          m_config->save();
          requestRefresh();
        }
      });
      controls->addChild(std::move(action));
      auto remove = std::make_unique<Button>();
      remove->setGlyph("x");
      remove->setVariant(ButtonVariant::Ghost);
      remove->setOnClick([this, i] {
        auto& list = m_config->mutableConfig().notifications.rules;
        if (i < list.size()) {
          list.erase(list.begin() + static_cast<std::ptrdiff_t>(i));
          m_config->save();
          requestRefresh();
        }
      });
      controls->addChild(std::move(remove));
      labelRow(rules[i].app, std::move(controls));
    }
    // Add a rule for an app that already sent notifications.
    std::vector<std::string> candidates;
    for (const auto& app : m_notifications->knownApps()) {
      if (std::ranges::none_of(rules, [&](const auto& r) { return lower(r.app) == lower(app); })) {
        candidates.push_back(app);
      }
    }
    if (candidates.empty()) {
      cardNode.addChild(makeLabel(rules.empty() ? "Apps show up here once they have sent a notification."
                                                : "Every app in the history has a rule.",
                                  theme::smallFontSize, theme::textTertiary));
    } else {
      auto controls = makeFlex(FlexDirection::Horizontal, 8.0F);
      controls->setAlign(FlexAlign::Center);
      auto app = std::make_unique<Select>();
      app->setOptions(candidates);
      app->setSelectedIndexSilently(0);
      app->setSize(180.0F, 0.0F);
      auto* appPtr = static_cast<Select*>(controls->addChild(std::move(app)));
      auto add = std::make_unique<Button>();
      add->setText("Add rule");
      add->setVariant(ButtonVariant::Secondary);
      add->setOnClick([this, appPtr, candidates] {
        const std::size_t index = appPtr->selectedIndex();
        if (index < candidates.size()) {
          m_config->mutableConfig().notifications.rules.push_back({candidates[index], "mute"});
          m_config->save();
          requestRefresh();
        }
      });
      controls->addChild(std::move(add));
      labelRow("New rule", std::move(controls));
    }
  } else if (setting.id == "sun-times") {
    const auto& location = cfg.location;
    std::string text = "Set a latitude and longitude (or fixed times) to switch by the sun.";
    if (location.customSchedule || (location.latitude && location.longitude)) {
      const auto eval = day_night_schedule::evaluate(location, std::nullopt, std::nullopt);
      const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(eval.untilBoundary).count();
      text = std::format("Now: {}. Next switch in {} h {} min.", eval.night ? "night" : "day", minutes / 60,
                         minutes % 60);
    }
    labelRow(setting.title, {}, text);
  } else if (setting.id == "power-info") {
    cardNode.addChild(makeLabel("Shut down, reboot and reboot to UEFI on the power page need a second click.",
                                theme::smallFontSize, theme::textSecondary));
    cardNode.addChild(makeLabel("The lock screen checks your password through PAM (the \"login\" stack).",
                                theme::smallFontSize, theme::textSecondary));
  }
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
      m_config->save();
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
      m_config->save();
    }
    requestRefresh();
    return true;
  }
  if (event.pressed && event.sym == XKB_KEY_Escape) {
    // Esc clears a search first, then closes.
    if (!m_query.empty() && m_search != nullptr) {
      m_search->setValue("");
      m_query.clear();
      requestRefresh();
    } else if (m_input.focusedArea() == nullptr || m_query.empty()) {
      close();
    }
    return true;
  }
  m_input.keyEvent(event.sym, event.utf32, event.modifiers, event.pressed, event.preedit);
  m_surface->requestRedraw();
  return true;
}
