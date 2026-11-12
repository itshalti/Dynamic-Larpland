#include "island/island.h"

#include "config/config.h"
#include "core/log.h"
#include "core/ui_phase.h"
#include "island/theme.h"
#include "island/wallpaper_page.h"
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

  // Every mode except "collapsed" has a page. Order = creation order of the page nodes.
  constexpr std::array<const char*, 5> kPageModes = {"expanded", "player", "calendar", "wallpapers", "power"};

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
    return mode == "collapsed" || std::ranges::any_of(kPageModes, [&](const char* m) { return mode == m; });
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

void Islands::initialize(
    WaylandConnection& wayland, RenderContext& renderContext, ConfigService& config, AsyncTextureCache& textures,
    WallpaperService& wallpapers
) {
  m_wayland = &wayland;
  m_renderContext = &renderContext;
  m_config = &config;
  m_textures = &textures;
  m_wallpapers = &wallpapers;
  m_time = formatTime(m_config->config().island.timeFormat);
  createInstances();
  m_clockTimer.startRepeating(std::chrono::seconds(1), [this] { updateClock(); });
}

void Islands::onOutputsChanged() {
  m_instances.clear();
  createInstances();
}

void Islands::onConfigChanged() { updateClock(); }

void Islands::onWallpapersChanged() {
  for (auto& inst : m_instances) {
    rebuildPage(*inst, "wallpapers");
  }
}

std::unique_ptr<Node> Islands::buildPage(Instance& inst, const std::string& mode) {
  const auto size = sizeFor(mode);
  Instance* raw = &inst;
  const PageContext ctx{
      .renderer = inst.surface->renderTarget().renderer(),
      .animations = inst.animations,
      .textures = *m_textures,
      .wallpapers = *m_wallpapers,
      .requestRedraw = [raw] { raw->surface->requestRedraw(); },
  };
  if (mode == "wallpapers") {
    return buildWallpaperPage(ctx, size.width, size.height);
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
  return page;
}

void Islands::rebuildPage(Instance& inst, const std::string& mode) {
  const auto it = inst.pages.find(mode);
  if (it == inst.pages.end() || inst.pill == nullptr) {
    return;
  }
  UiPhaseScope layoutPhase(UiPhase::Layout);
  const float opacity = it->second->opacity();
  inst.animations.cancelForOwner(it->second);
  inst.pill->removeChild(it->second);
  auto page = buildPage(inst, mode);
  page->setOpacity(opacity);
  it->second = inst.pill->addChild(std::move(page));
  applyGeometry(inst);
}

void Islands::createInstances() {
  for (const auto& output : m_wayland->outputs()) {
    if (!output.done || output.output == nullptr || !output.hasUsableGeometry()) {
      continue;
    }
    auto inst = std::make_unique<Instance>();
    inst->output = output.output;
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

  auto clock = std::make_unique<Label>();
  clock->setFontSize(theme::clockFontSize);
  clock->setColor(theme::textPrimary);
  clock->setText(m_time);
  inst.clock = static_cast<Label*>(inst.pill->addChild(std::move(clock)));

  // Each page is laid out at its mode's final size, so nothing reflows while the pill morphs.
  for (const char* mode : kPageModes) {
    auto page = buildPage(inst, mode);
    page->setOpacity(0.0F);
    // Hidden pages must not take clicks.
    page->setVisible(false);
    inst.pages[mode] = inst.pill->addChild(std::move(page));
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
  if (inst.clock != nullptr) {
    inst.clock->measure(inst.surface->renderTarget().renderer());
    inst.clock->setPosition(
        (inst.width - inst.clock->width()) / 2.0F, (theme::collapsedSize.height - inst.clock->height()) / 2.0F
    );
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
  inst.mode = mode;
  inst.modeChangedAt = std::chrono::steady_clock::now();

  // Morph width, height and radius together.
  const auto target = sizeFor(mode);
  const float targetRadius = mode == "collapsed" ? target.height / 2.0F : theme::expandedRadius;
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
    if (m == "collapsed") {
      return raw->clock;
    }
    const auto it = raw->pages.find(m);
    return it != raw->pages.end() ? it->second : nullptr;
  };
  fade(content(previous), 0.0F, theme::animFadeMs);
  fade(content(mode), 1.0F, theme::animFadeMs * 1.6F);
  inst.surface->requestRedraw();
}

void Islands::scheduleClose(Instance& inst) {
  const auto sinceSwitch = std::chrono::steady_clock::now() - inst.modeChangedAt;
  const int delay = sinceSwitch < std::chrono::milliseconds(theme::modeSwitchGraceMs) ? theme::modeSwitchCloseDelayMs
                                                                                        : theme::hoverCloseDelayMs;
  Instance* raw = &inst;
  inst.closeTimer.start(std::chrono::milliseconds(delay), [this, raw] {
    if (!raw->hovered) {
      setMode(*raw, "collapsed");
    }
  });
}

void Islands::setMode(const std::string& mode) {
  if (!isMode(mode)) {
    return;
  }
  for (auto& inst : m_instances) {
    setMode(*inst, mode);
  }
}

void Islands::toggleMode(const std::string& mode) {
  for (auto& inst : m_instances) {
    setMode(*inst, inst->mode == mode ? std::string("collapsed") : mode);
  }
}

void Islands::toggle() {
  for (auto& inst : m_instances) {
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
    if (inst->clock != nullptr) {
      inst->clock->setText(m_time);
      applyGeometry(*inst);
    }
  }
}

bool Islands::onPointerEvent(const PointerEvent& event) {
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
