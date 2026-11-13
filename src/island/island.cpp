#include "island/island.h"

#include "config/config.h"
#include "core/deferred_call.h"
#include "core/log.h"
#include "core/ui_phase.h"
#include "dbus/tray_service.h"
#include "ui/controls/context_menu_popup.h"
#include "wayland/layer_shell.h"
#include "island/theme.h"
#include "island/collapsed_page.h"
#include "island/expanded_page.h"
#include "island/media_pages.h"
#include "island/power_page.h"
#include "island/wallpaper_page.h"
#include "services/media_service.h"
#include "render/render_context.h"
#include "ui/controls/box.h"
#include "ui/controls/label.h"
#include "wayland/wayland_connection.h"
#include "wayland/wayland_seat.h"

#include <algorithm>
#include <array>
#include <ctime>
#include <xkbcommon/xkbcommon-keysyms.h>

namespace {

  constexpr Logger kLog("island");

  // Every mode has a page. Order = creation order of the page nodes.
  constexpr std::array<const char*, 6> kPageModes = {"collapsed", "expanded",   "player",
                                                     "calendar",  "wallpapers", "power"};

  theme::Size sizeFor(const std::string& mode) {
    if (mode == "expanded") {
      return theme::expandedSize;
    }
    if (mode == "player") {
      return theme::playerSize;
    }
    if (mode == "calendar") {
      return theme::calendarSize;
    }
    if (mode == "wallpapers") {
      return theme::wallpapersSize;
    }
    if (mode == "power") {
      return theme::powerSize;
    }
    return theme::collapsedSize;
  }

  bool isMode(const std::string& mode) {
    return std::ranges::any_of(kPageModes, [&](const char* m) { return mode == m; });
  }

  void cancelAnimations(AnimationManager& animations, Node* node) {
    animations.cancelForOwner(node);
    for (const auto& child : node->children()) {
      cancelAnimations(animations, child.get());
    }
  }

  std::string formatTime(const std::string& format) {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_r(&now, &local);
    std::array<char, 64> buf{};
    const std::size_t n = std::strftime(buf.data(), buf.size(), format.c_str(), &local);
    return n > 0 ? std::string(buf.data(), n) : std::string("--:--");
  }

} // namespace

Islands::Islands() = default;
Islands::~Islands() = default;

void Islands::initialize(
    WaylandConnection& wayland, RenderContext& renderContext, ConfigService& config, AsyncTextureCache& textures,
    WallpaperService& wallpapers, MediaService& media, NotificationService& notifications, TrayService& tray,
    IconResolver& icons, RecorderService& recorder
) {
  m_recorder = &recorder;
  m_tray = &tray;
  m_icons = &icons;
  m_trayMenu = std::make_unique<ContextMenuPopup>(wayland, renderContext);
  m_media = &media;
  m_notifications = &notifications;
  m_wayland = &wayland;
  m_renderContext = &renderContext;
  m_config = &config;
  m_textures = &textures;
  m_wallpapers = &wallpapers;
  m_time = formatTime(m_config->config().island.timeFormat);
  createInstances();
  m_clockTimer.startRepeating(std::chrono::seconds(1), [this] { updateClock(); });
  // Live values (playback position) of the page each island shows.
  m_tickTimer.startRepeating(std::chrono::milliseconds(250), [this] {
    for (auto& inst : m_instances) {
      const auto it = inst->ticks.find(inst->mode);
      if (it != inst->ticks.end() && it->second) {
        it->second();
      }
    }
  });
}

void Islands::onOutputsChanged() {
  m_instances.clear();
  createInstances();
}

void Islands::onConfigChanged() {
  updateClock();
  // DND, first day of week and other options show on the pages.
  for (auto& inst : m_instances) {
    rebuildPage(*inst, "expanded");
    rebuildPage(*inst, "calendar");
  }
}

void Islands::onMediaChanged() {
  for (auto& inst : m_instances) {
    rebuildPage(*inst, "expanded");
    rebuildPage(*inst, "player");
    refreshCollapsed(*inst);
  }
}

void Islands::onNotificationPosted(const NotificationEntry& entry) {
  for (auto& inst : m_instances) {
    if (inst->mode != "collapsed") {
      continue;
    }
    inst->popup = entry;
    Instance* raw = inst.get();
    inst->popupTimer.start(std::chrono::milliseconds(m_config->config().notifications.popupTimeoutMs), [this, raw] {
      raw->popup.reset();
      refreshCollapsed(*raw);
    });
    refreshCollapsed(*inst);
  }
}

theme::Size Islands::targetSize(const Instance& inst, const std::string& mode) const {
  if (mode != "collapsed") {
    return sizeFor(mode);
  }
  if (inst.popup) {
    return theme::collapsedNotifSize;
  }
  return m_media->playing() ? theme::collapsedMediaSize : theme::collapsedSize;
}

void Islands::refreshCollapsed(Instance& inst) {
  rebuildPage(inst, "collapsed");
  if (inst.mode == "collapsed") {
    morphTo(inst);
  }
}

void Islands::onRecorderChanged() {
  for (auto& inst : m_instances) {
    rebuildPage(*inst, "expanded");
  }
}

std::string Islands::targetConnector() {
  const auto list = targets();
  return list.size() == 1 ? list.front()->connector : std::string();
}

void Islands::onTrayChanged() {
  for (auto& inst : m_instances) {
    rebuildPage(*inst, "expanded");
  }
  // Entries of a requested menu arrived: open it now (unless the request is stale).
  if (m_pendingMenu && std::chrono::steady_clock::now() - m_pendingMenu->requestedAt < std::chrono::seconds(2)) {
    const PendingMenu pending = *m_pendingMenu;
    m_pendingMenu.reset();
    openTrayMenu(*pending.inst, pending.itemId, pending.x, pending.y, pending.w, pending.h, pending.parentId);
  }
}

void Islands::openTrayMenu(
    Instance& inst, const std::string& itemId, float x, float y, float w, float h, std::int32_t parentId
) {
  std::vector<ContextMenuControlEntry> entries;
  for (const auto& e : parentId == 0 ? m_tray->menuEntries(itemId) : m_tray->menuEntriesForParent(itemId, parentId)) {
    if (!e.visible) {
      continue;
    }
    entries.push_back(ContextMenuControlEntry{
        .id = e.id,
        .label = e.label,
        .enabled = e.enabled,
        .separator = e.separator,
        .hasSubmenu = e.hasSubmenu,
        .checkmark = e.checkmark,
        .radio = e.radio,
        .toggleState = e.toggleState,
    });
  }
  if (entries.empty()) {
    // The service is fetching the layout; retry once it reports the change.
    m_pendingMenu = PendingMenu{&inst, itemId, x, y, w, h, parentId, std::chrono::steady_clock::now()};
    return;
  }
  m_pendingMenu.reset();
  // Page coordinates -> surface coordinates (pages sit centered in the pill).
  const auto pageIt = inst.pages.find(inst.mode);
  const float offsetX = inst.pill->x() + (pageIt != inst.pages.end() ? pageIt->second->x() : 0.0F);
  Instance* raw = &inst;
  m_trayMenu->setOnActivate([this, raw, itemId, x, y, w, h](const ContextMenuControlEntry& entry) {
    if (entry.hasSubmenu) {
      DeferredCall::callLater([this, raw, itemId, x, y, w, h, id = entry.id] {
        openTrayMenu(*raw, itemId, x, y, w, h, id);
      });
      return;
    }
    (void)m_tray->activateMenuEntry(itemId, entry.id);
  });
  m_trayMenu->setOnDismissed([this, raw, itemId] {
    m_menuOpen = false;
    m_tray->notifyMenuClosed(itemId);
    if (!raw->hovered && raw->mode != "collapsed") {
      scheduleClose(*raw);
    }
  });
  auto& layerShell = m_wayland->extension<LayerShell>();
  m_trayMenu->open(ContextMenuPopupRequest{
      .entries = std::move(entries),
      .minMenuWidth = 180.0F,
      .maxMenuWidth = 340.0F,
      .anchor = {.x = static_cast<std::int32_t>(offsetX + x),
                 .y = static_cast<std::int32_t>(inst.pill->y() + y),
                 .width = static_cast<std::int32_t>(w),
                 .height = static_cast<std::int32_t>(h)},
      .parent = {.attach = LayerShell::popupAttach(inst.surface->layerSurface()),
                 .output = inst.output,
                 .keyboardGrab = layerShell.popupKeyboardGrab(inst.surface->wlSurface())},
      .pointerParentSurface = inst.surface->wlSurface(),
  });
  m_menuOpen = m_trayMenu->isOpen();
  inst.closeTimer.stop();
  m_tray->notifyMenuOpened(itemId, parentId);
}

void Islands::onNotificationsChanged() {
  for (auto& inst : m_instances) {
    rebuildPage(*inst, "expanded");
  }
}

void Islands::runAction(Instance& inst, const std::string& action) {
  if (action.starts_with("rebuild:")) {
    rebuildPage(inst, action.substr(8));
  } else if (isMode(action)) {
    setMode(inst, action);
  } else if (m_actionHandler) {
    m_actionHandler(action);
  }
}

void Islands::onWallpapersChanged() {
  // The palette (accent) may have changed too, and every page uses it.
  for (auto& inst : m_instances) {
    for (const char* mode : kPageModes) {
      rebuildPage(*inst, mode);
    }
  }
}

BuiltPage Islands::buildPage(Instance& inst, const std::string& mode) {
  const auto size = targetSize(inst, mode);
  Instance* raw = &inst;
  const PageContext ctx{
      .renderer = inst.surface->renderTarget().renderer(),
      .animations = inst.animations,
      .textures = *m_textures,
      .wallpapers = *m_wallpapers,
      .media = *m_media,
      .notifications = *m_notifications,
      .tray = *m_tray,
      .recorder = *m_recorder,
      .icons = *m_icons,
      .requestRedraw = [raw] { raw->surface->requestRedraw(); },
      // Deferred: an action may rebuild the page whose button triggered it.
      .action = [this, raw](const std::string& action) {
        DeferredCall::callLater([this, raw, action] { runAction(*raw, action); });
      },
      .openTrayMenu =
          [this, raw](const std::string& itemId, float x, float y, float w, float h) {
            DeferredCall::callLater([this, raw, itemId, x, y, w, h] { openTrayMenu(*raw, itemId, x, y, w, h); });
          },
      .firstDayOfWeek = m_config->config().calendar.firstDayOfWeek,
      .dnd = m_config->config().notifications.dnd,
  };
  if (mode == "collapsed") {
    return buildCollapsedPage(ctx, m_time, inst.popup ? &*inst.popup : nullptr, size.width, size.height);
  }
  if (mode == "calendar") {
    return buildMonthPage(ctx, m_calendar, size.width, size.height);
  }
  if (mode == "wallpapers") {
    return buildWallpaperPage(ctx, size.width, size.height);
  }
  if (mode == "expanded") {
    return buildExpandedPage(ctx, m_calendar, size.width, size.height);
  }
  if (mode == "player") {
    return buildPlayerPage(ctx, size.width, size.height);
  }
  if (mode == "power") {
    return buildPowerPage(ctx, size.width, size.height);
  }
  // shortcut: placeholder title until the real page content lands (milestones 4-5).
  auto page = std::make_unique<Node>();
  page->setSize(size.width, size.height);
  auto title = std::make_unique<Label>();
  title->setFontSize(theme::clockFontSize);
  title->setColor(theme::textSecondary);
  title->setText(mode);
  title->measure(ctx.renderer);
  title->setPosition(28.0F, 20.0F);
  page->addChild(std::move(title));
  return {.node = std::move(page), .tick = {}};
}

void Islands::rebuildPage(Instance& inst, const std::string& mode) {
  const auto it = inst.pages.find(mode);
  if (it == inst.pages.end() || inst.pill == nullptr) {
    return;
  }
  UiPhaseScope layoutPhase(UiPhase::Layout);
  // Animations hold raw node pointers (owner = node): stop every one in the subtree before it is destroyed.
  cancelAnimations(inst.animations, it->second);
  auto page = buildPage(inst, mode);
  // A rebuild lands in the final state of the current mode (a fade in progress is not resumed).
  const bool shown = inst.mode == mode;
  page.node->setOpacity(shown ? 1.0F : 0.0F);
  page.node->setVisible(shown);
  inst.ticks[mode] = std::move(page.tick);
  inst.pill->removeChild(it->second);
  it->second = inst.pill->addChild(std::move(page.node));
  applyGeometry(inst);
}

void Islands::createInstances() {
  for (const auto& output : m_wayland->outputs()) {
    if (!output.done || output.output == nullptr || !output.hasUsableGeometry()) {
      continue;
    }
    auto inst = std::make_unique<Instance>();
    inst->output = output.output;
    inst->connector = output.connectorName;
    inst->width = theme::collapsedSize.width;
    inst->height = theme::collapsedSize.height;
    inst->radius = theme::collapsedSize.height / 2.0F;

    inst->surface = std::make_unique<LayerSurface>(
        *m_wayland,
        LayerSurfaceConfig{
            .nameSpace = "dynamic-larpland",
            .layer = LayerShellLayer::Overlay,
            .anchor = LayerShellAnchor::Top,
            .width = static_cast<std::uint32_t>(theme::windowWidth),
            .height = static_cast<std::uint32_t>(theme::windowHeight),
            .exclusiveZone = -1,
            .marginTop = theme::topMargin,
            // Constant on purpose: switching keyboard interactivity mid-hover makes Hyprland send a
            // spurious pointer leave, which collapsed the island right after it opened.
            .keyboard = LayerShellKeyboard::OnDemand,
            .defaultWidth = static_cast<std::uint32_t>(theme::windowWidth),
            .defaultHeight = static_cast<std::uint32_t>(theme::windowHeight),
        }
    );
    inst->surface->setRenderContext(m_renderContext);
    inst->surface->setAnimationManager(&inst->animations);

    Instance* raw = inst.get();
    inst->surface->setConfigureCallback([raw](std::uint32_t, std::uint32_t) { raw->surface->requestLayout(); });
    inst->surface->setPrepareFrameCallback([this, raw](bool, bool) {
      if (raw->root == nullptr) {
        UiPhaseScope layoutPhase(UiPhase::Layout);
        buildScene(*raw);
      }
    });

    if (!inst->surface->initialize(output.output)) {
      kLog.warn("could not create the island surface on {}", output.connectorName);
      continue;
    }
    applyGeometry(*inst);
    m_instances.push_back(std::move(inst));
  }
}

void Islands::buildScene(Instance& inst) {
  auto root = std::make_unique<Node>();
  root->setSize(static_cast<float>(theme::windowWidth), static_cast<float>(theme::windowHeight));

  auto pill = std::make_unique<Box>();
  pill->setFill(theme::surface);
  pill->setClipChildren(true);
  inst.pill = static_cast<Box*>(root->addChild(std::move(pill)));

  // Each page is laid out at its mode's final size, so nothing reflows while the pill morphs.
  for (const char* mode : kPageModes) {
    auto page = buildPage(inst, mode);
    const bool shown = inst.mode == mode;
    page.node->setOpacity(shown ? 1.0F : 0.0F);
    // Hidden pages must not take clicks.
    page.node->setVisible(shown);
    inst.ticks[mode] = std::move(page.tick);
    inst.pages[mode] = inst.pill->addChild(std::move(page.node));
  }

  inst.root = std::move(root);
  inst.surface->setSceneRoot(inst.root.get());
  inst.input.setSceneRoot(inst.root.get());
  inst.input.setCursorShapeCallback([this](std::uint32_t serial, std::uint32_t shape) {
    m_wayland->setCursorShape(serial, shape);
  });
  applyGeometry(inst);
}

void Islands::applyGeometry(Instance& inst) {
  const float x = (static_cast<float>(theme::windowWidth) - inst.width) / 2.0F;
  if (inst.pill != nullptr) {
    inst.pill->setPosition(x, 0.0F);
    inst.pill->setSize(inst.width, inst.height);
    inst.pill->setRadius(inst.radius);
  }
  for (auto& [mode, page] : inst.pages) {
    page->setPosition((inst.width - page->width()) / 2.0F, 0.0F);
  }
  // Only the pill takes input; the rest of the window is click-through.
  inst.surface->setInputRegion({InputRect{
      .x = static_cast<int>(x),
      .y = 0,
      .width = static_cast<int>(inst.width),
      .height = static_cast<int>(inst.height),
  }});
  inst.surface->requestRedraw();
}

void Islands::setMode(Instance& inst, const std::string& mode) {
  inst.openTimer.stop();
  inst.closeTimer.stop();
  if (inst.mode == mode) {
    return;
  }
  const std::string previous = inst.mode;
  // Calendar views start at today whenever they open again (rebuilt while still hidden, so they fade in).
  if (mode == "expanded" && previous == "collapsed" && m_calendar.stripOffset != 0) {
    m_calendar.stripOffset = 0;
    rebuildPage(inst, "expanded");
  }
  if (mode == "calendar") {
    m_calendar.shownMonth = {};
    rebuildPage(inst, "calendar");
  }
  // Opening the island replaces the notification live activity.
  if (previous == "collapsed" && inst.popup) {
    inst.popup.reset();
    inst.popupTimer.stop();
    rebuildPage(inst, "collapsed");
  }
  inst.mode = mode;
  inst.modeChangedAt = std::chrono::steady_clock::now();
  morphTo(inst);
  Instance* raw = &inst;

  // Cross-fade: the old content fades out fast, the new one a little slower.
  auto fade = [raw](Node* node, float to, float durationMs) {
    if (node == nullptr) {
      return;
    }
    raw->animations.cancelForOwner(node);
    node->setVisible(true);
    raw->animations.animate(
        node->opacity(), to, durationMs, Easing::EaseOutQuad,
        [raw, node](float v) {
          node->setOpacity(v);
          raw->surface->requestRedraw();
        },
        [node, to] { node->setVisible(to > 0.0F); }, node
    );
  };
  auto content = [raw](const std::string& m) -> Node* {
    const auto it = raw->pages.find(m);
    return it != raw->pages.end() ? it->second : nullptr;
  };
  fade(content(previous), 0.0F, theme::animFadeMs);
  fade(content(mode), 1.0F, theme::animFadeMs * 1.6F);
  inst.surface->requestRedraw();
}

void Islands::morphTo(Instance& inst) {
  // Morph width, height and radius together.
  const auto target = targetSize(inst, inst.mode);
  const float targetRadius = inst.mode == "collapsed" ? target.height / 2.0F : theme::expandedRadius;
  const float fromW = inst.width;
  const float fromH = inst.height;
  const float fromR = inst.radius;
  if (inst.morph != 0) {
    inst.animations.cancel(inst.morph);
  }
  Instance* raw = &inst;
  inst.morph = inst.animations.animate(
      0.0F, 1.0F, theme::animMorphMs, Easing::EaseOutCubic,
      [this, raw, fromW, fromH, fromR, target, targetRadius](float t) {
        raw->width = fromW + (target.width - fromW) * t;
        raw->height = fromH + (target.height - fromH) * t;
        raw->radius = fromR + (targetRadius - fromR) * t;
        applyGeometry(*raw);
      },
      [raw] { raw->morph = 0; }, raw
  );
  inst.surface->requestRedraw();
}

void Islands::scheduleClose(Instance& inst) {
  const auto sinceSwitch = std::chrono::steady_clock::now() - inst.modeChangedAt;
  const int delay = sinceSwitch < std::chrono::milliseconds(theme::modeSwitchGraceMs) ? theme::modeSwitchCloseDelayMs
                                                                                        : theme::hoverCloseDelayMs;
  Instance* raw = &inst;
  inst.closeTimer.start(std::chrono::milliseconds(delay), [this, raw] {
    if (!raw->hovered && !m_menuOpen) {
      setMode(*raw, "collapsed");
    }
  });
}

std::vector<Islands::Instance*> Islands::targets() {
  const std::string focused = m_focusedMonitor ? m_focusedMonitor() : std::string();
  std::vector<Instance*> out;
  for (auto& inst : m_instances) {
    if (focused.empty() || inst->connector == focused) {
      out.push_back(inst.get());
    }
  }
  // Unknown connector names (e.g. a nested session): fall back to every island.
  if (out.empty()) {
    for (auto& inst : m_instances) {
      out.push_back(inst.get());
    }
  }
  return out;
}

void Islands::setMode(const std::string& mode) {
  if (!isMode(mode)) {
    return;
  }
  for (Instance* inst : targets()) {
    setMode(*inst, mode);
  }
}

void Islands::toggleMode(const std::string& mode) {
  for (Instance* inst : targets()) {
    setMode(*inst, inst->mode == mode ? std::string("collapsed") : mode);
  }
}

void Islands::toggle() {
  for (Instance* inst : targets()) {
    setMode(*inst, inst->mode == "collapsed" ? std::string("expanded") : std::string("collapsed"));
  }
}

void Islands::updateClock() {
  const std::string now = formatTime(m_config->config().island.timeFormat);
  if (now == m_time) {
    return;
  }
  m_time = now;
  for (auto& inst : m_instances) {
    rebuildPage(*inst, "collapsed");
  }
}

bool Islands::onPointerEvent(const PointerEvent& event) {
  if (m_trayMenu && m_trayMenu->onPointerEvent(event)) {
    return true;
  }
  for (auto& inst : m_instances) {
    if (event.surface != inst->surface->wlSurface()) {
      continue;
    }
    Instance& i = *inst;
    const auto x = static_cast<float>(event.sx);
    const auto y = static_cast<float>(event.sy);
    switch (event.type) {
    case PointerEvent::Type::Enter:
      i.input.pointerEnter(x, y, event.serial);
      i.hovered = true;
      i.closeTimer.stop();
      if (i.mode == "collapsed" && m_config->config().island.hoverExpand) {
        Instance* raw = &i;
        i.openTimer.start(std::chrono::milliseconds(m_config->config().island.hoverDelayMs), [this, raw] {
          setMode(*raw, "expanded");
        });
      }
      break;
    case PointerEvent::Type::Leave:
      i.input.pointerLeave();
      i.hovered = false;
      i.openTimer.stop();
      if (i.mode != "collapsed") {
        scheduleClose(i);
      }
      break;
    case PointerEvent::Type::Motion:
      i.input.pointerMotion(x, y, event.serial);
      break;
    case PointerEvent::Type::Button:
      if (event.pressed && i.mode == "collapsed") {
        setMode(i, "expanded");
        break;
      }
      i.input.pointerButton(x, y, event.button, event.pressed, event.serial, event.time, event.touch);
      break;
    case PointerEvent::Type::Axis:
      i.input.pointerAxis(
          x, y, event.axis, event.axisSource, event.axisValue, event.axisDiscrete, event.axisValue120,
          event.axisLines, event.axisGestureSerial
      );
      break;
    default:
      break;
    }
    return true;
  }
  return false;
}

bool Islands::onKeyboardEvent(const KeyboardEvent& event) {
  if (m_trayMenu && m_trayMenu->isOpen()) {
    m_trayMenu->onKeyboardEvent(event);
    return true;
  }
  if (!event.pressed || event.sym != XKB_KEY_Escape) {
    return false;
  }
  bool handled = false;
  for (auto& inst : m_instances) {
    if (inst->mode != "collapsed") {
      setMode(*inst, "collapsed");
      handled = true;
    }
  }
  return handled;
}
