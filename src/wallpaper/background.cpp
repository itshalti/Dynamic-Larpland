#include "wallpaper/background.h"

#include "core/ui_phase.h"
#include "island/theme.h"
#include "render/core/async_texture_cache.h"
#include "services/wallpaper_service.h"
#include "ui/controls/image.h"
#include "wayland/wayland_connection.h"

void Backgrounds::initialize(
    WaylandConnection& wayland, RenderContext& renderContext, AsyncTextureCache& textures, WallpaperService& wallpapers
) {
  m_wayland = &wayland;
  m_renderContext = &renderContext;
  m_textures = &textures;
  m_wallpapers = &wallpapers;
  createInstances();
}

void Backgrounds::onOutputsChanged() {
  m_instances.clear();
  createInstances();
}

void Backgrounds::refresh() {
  for (auto& inst : m_instances) {
    show(*inst);
  }
}

void Backgrounds::createInstances() {
  for (const auto& output : m_wayland->outputs()) {
    if (!output.done || output.output == nullptr || !output.hasUsableGeometry()) {
      continue;
    }
    auto inst = std::make_unique<Instance>();
    inst->output = output.output;
    inst->surface = std::make_unique<LayerSurface>(
        *m_wayland,
        LayerSurfaceConfig{
            .nameSpace = "dynamic-larpland-wallpaper",
            .layer = LayerShellLayer::Background,
            .anchor = LayerShellAnchor::Top | LayerShellAnchor::Bottom | LayerShellAnchor::Left
                      | LayerShellAnchor::Right,
            .exclusiveZone = -1,
            .keyboard = LayerShellKeyboard::None,
        }
    );
    inst->surface->setRenderContext(m_renderContext);
    inst->surface->setAnimationManager(&inst->animations);
    Instance* raw = inst.get();
    inst->surface->setConfigureCallback([raw](std::uint32_t, std::uint32_t) { raw->surface->requestLayout(); });
    inst->surface->setPrepareFrameCallback([this, raw](bool, bool) {
      UiPhaseScope layoutPhase(UiPhase::Layout);
      const auto w = static_cast<float>(raw->surface->width());
      const auto h = static_cast<float>(raw->surface->height());
      if (raw->root == nullptr) {
        buildScene(*raw);
      }
      if (raw->root->width() != w || raw->root->height() != h) {
        raw->root->setSize(w, h);
        raw->back->setSize(w, h);
        raw->front->setSize(w, h);
      }
    });
    if (!inst->surface->initialize(output.output)) {
      continue;
    }
    inst->surface->setInputRegion({});
    m_instances.push_back(std::move(inst));
  }
}

void Backgrounds::buildScene(Instance& inst) {
  inst.root = std::make_unique<Node>();
  auto makeImage = [&] {
    auto image = std::make_unique<Image>();
    image->setFit(ImageFit::Cover);
    return static_cast<Image*>(inst.root->addChild(std::move(image)));
  };
  inst.back = makeImage();
  inst.front = makeImage();
  inst.surface->setSceneRoot(inst.root.get());
  show(inst);
}

void Backgrounds::show(Instance& inst) {
  const std::string& path = m_wallpapers->current();
  if (inst.root == nullptr || path == inst.shown) {
    return;
  }
  inst.shown = path;
  Instance* raw = &inst;
  // The new image fades in over the old one once decoded; then it becomes the back layer.
  raw->front->setOpacity(0.0F);
  raw->front->setAsyncReadyCallback([raw, path] {
    raw->animations.cancelForOwner(raw->front);
    raw->animations.animate(
        0.0F, 1.0F, theme::animWallpaperMs, Easing::EaseOutQuad,
        [raw](float v) {
          raw->front->setOpacity(v);
          raw->surface->requestRedraw();
        },
        [raw] { std::swap(raw->back, raw->front); }, raw->front
    );
  });
  raw->front->setZIndex(1);
  raw->back->setZIndex(0);
  raw->front->setSourceFileAsync(raw->surface->renderTarget().renderer(), *m_textures, path);
  raw->surface->requestRedraw();
}
