#pragma once

#include "core/timer_manager.h"
#include "render/animation/animation_manager.h"
#include "render/scene/node.h"
#include "wayland/layer_surface.h"

#include <memory>
#include <string>
#include <vector>

class Box;
class Label;
class RenderContext;
class WaylandConnection;
struct PointerEvent;
struct wl_output;

// The island: one transparent layer surface per output at the top center, holding the animated
// black pill. Collapsed it shows the clock; toggle() morphs it open and closed.
class Islands {
public:
  void initialize(WaylandConnection& wayland, RenderContext& renderContext);
  // Re-creates the per-output surfaces after outputs were added or removed.
  void onOutputsChanged();
  bool onPointerEvent(const PointerEvent& event);

  void toggle();
  void setExpanded(bool expanded);
  bool expanded() const noexcept { return m_expanded; }

private:
  struct Instance {
    wl_output* output = nullptr;
    std::unique_ptr<LayerSurface> surface;
    AnimationManager animations;
    std::unique_ptr<Node> root;
    Box* pill = nullptr;
    Label* clock = nullptr;
    float width = 0.0F;
    float height = 0.0F;
    float radius = 0.0F;
    AnimationManager::Id morph = 0;
  };

  void createInstances();
  void buildScene(Instance& inst);
  void applyGeometry(Instance& inst);
  void animateTo(Instance& inst);
  void updateClock();

  WaylandConnection* m_wayland = nullptr;
  RenderContext* m_renderContext = nullptr;
  std::vector<std::unique_ptr<Instance>> m_instances;
  std::string m_time;
  Timer m_clockTimer;
  bool m_expanded = false;
};
