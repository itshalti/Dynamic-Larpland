#pragma once

#include "core/timer_manager.h"
#include "render/animation/animation_manager.h"
#include "render/scene/input_dispatcher.h"
#include "render/scene/node.h"
#include "wayland/layer_surface.h"

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class AsyncTextureCache;
class Box;
class ConfigService;
class Label;
class RenderContext;
class WallpaperService;
class WaylandConnection;
struct KeyboardEvent;
struct PointerEvent;
struct wl_output;

// The island: one transparent layer surface per output at the top center, holding the animated
// black pill. Each output has its own mode:
//   collapsed | expanded | player | calendar | wallpapers | power
// Every mode has a target size (island/theme.h); the pill morphs to it and the page for the mode
// cross-fades in. Hover expands after island.hover_delay_ms, leaving collapses after a short delay.
class Islands {
public:
  void initialize(
      WaylandConnection& wayland, RenderContext& renderContext, ConfigService& config, AsyncTextureCache& textures,
      WallpaperService& wallpapers
  );
  // Re-creates the per-output surfaces after outputs were added or removed.
  void onOutputsChanged();
  void onConfigChanged();
  // Wallpaper list, current wallpaper or palette changed: rebuild the pages that show them.
  void onWallpapersChanged();
  bool onPointerEvent(const PointerEvent& event);
  bool onKeyboardEvent(const KeyboardEvent& event);

  // Actions from IPC/keybinds. shortcut: they act on every output until the focused monitor is
  // tracked (Hyprland IPC, milestone 4); then only the focused output's island reacts.
  void toggle();
  void setMode(const std::string& mode);
  // Opens `mode`, or collapses if it is already open.
  void toggleMode(const std::string& mode);

private:
  struct Instance {
    wl_output* output = nullptr;
    std::unique_ptr<LayerSurface> surface;
    AnimationManager animations;
    InputDispatcher input;
    std::unique_ptr<Node> root;
    Box* pill = nullptr;
    Label* clock = nullptr;
    std::unordered_map<std::string, Node*> pages;
    std::string mode = "collapsed";
    std::chrono::steady_clock::time_point modeChangedAt;
    bool hovered = false;
    float width = 0.0F;
    float height = 0.0F;
    float radius = 0.0F;
    AnimationManager::Id morph = 0;
    Timer openTimer;
    Timer closeTimer;
  };

  void createInstances();
  void buildScene(Instance& inst);
  std::unique_ptr<Node> buildPage(Instance& inst, const std::string& mode);
  void rebuildPage(Instance& inst, const std::string& mode);
  void applyGeometry(Instance& inst);
  void setMode(Instance& inst, const std::string& mode);
  void scheduleClose(Instance& inst);
  void updateClock();

  WaylandConnection* m_wayland = nullptr;
  RenderContext* m_renderContext = nullptr;
  ConfigService* m_config = nullptr;
  AsyncTextureCache* m_textures = nullptr;
  WallpaperService* m_wallpapers = nullptr;
  std::vector<std::unique_ptr<Instance>> m_instances;
  std::string m_time;
  Timer m_clockTimer;
};
