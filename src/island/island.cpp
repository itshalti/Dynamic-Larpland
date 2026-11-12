#include "island/island.h"

#include "core/log.h"
#include "core/ui_phase.h"
#include "island/theme.h"
#include "render/render_context.h"
#include "render/scene/node.h"
#include "ui/controls/box.h"
#include "ui/controls/label.h"
#include "wayland/wayland_connection.h"
#include "wayland/wayland_seat.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <format>

namespace {

  constexpr Logger kLog("island");

  std::string currentTime() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_r(&now, &local);
    return std::format("{:02}:{:02}", local.tm_hour, local.tm_min);
  }

} // namespace

void Islands::initialize(WaylandConnection& wayland, RenderContext& renderContext) {
  m_wayland = &wayland;
  m_renderContext = &renderContext;
  m_time = currentTime();
  createInstances();
  m_clockTimer.startRepeating(std::chrono::seconds(1), [this] { updateClock(); });
}

void Islands::onOutputsChanged() {
  m_instances.clear();
  createInstances();
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
            .keyboard = LayerShellKeyboard::None,
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
  inst.pill = static_cast<Box*>(root->addChild(std::move(pill)));

  auto clock = std::make_unique<Label>();
  clock->setFontSize(theme::clockFontSize);
  clock->setColor(theme::textPrimary);
  clock->setText(m_time);
  inst.clock = static_cast<Label*>(inst.pill->addChild(std::move(clock)));

  inst.root = std::move(root);
  inst.surface->setSceneRoot(inst.root.get());
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
    // The clock belongs to the collapsed pill; it fades out as the pill opens.
    const float progress = (inst.width - theme::collapsedSize.width)
                           / (theme::expandedSize.width - theme::collapsedSize.width);
    inst.clock->setOpacity(std::clamp(1.0F - progress * 3.0F, 0.0F, 1.0F));
    inst.clock->setPosition(
        (inst.width - inst.clock->width()) / 2.0F, (theme::collapsedSize.height - inst.clock->height()) / 2.0F
    );
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

void Islands::animateTo(Instance& inst) {
  const auto target = m_expanded ? theme::expandedSize : theme::collapsedSize;
  const float targetRadius = m_expanded ? theme::expandedRadius : theme::collapsedSize.height / 2.0F;
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

void Islands::setExpanded(bool expanded) {
  if (m_expanded == expanded) {
    return;
  }
  m_expanded = expanded;
  for (auto& inst : m_instances) {
    animateTo(*inst);
  }
}

void Islands::toggle() { setExpanded(!m_expanded); }

void Islands::updateClock() {
  const std::string now = currentTime();
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
    if (event.type == PointerEvent::Type::Button && event.pressed) {
      toggle();
    }
    return true;
  }
  return false;
}
