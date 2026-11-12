#pragma once

#include "render/animation/animation_manager.h"
#include "render/scene/node.h"
#include "wayland/layer_surface.h"

#include <memory>
#include <string>
#include <vector>

class AsyncTextureCache;
class Image;
class RenderContext;
class WallpaperService;
class WaylandConnection;
struct wl_output;

// The desktop wallpaper: one Background-layer surface per output drawing WallpaperService::current(),
// cross-fading to a newly picked image once it has decoded.
class Backgrounds {
public:
  void initialize(
      WaylandConnection& wayland, RenderContext& renderContext, AsyncTextureCache& textures, WallpaperService& wallpapers
  );
  void onOutputsChanged();
  // Shows the current wallpaper (no-op when it didn't change).
  void refresh();

private:
  struct Instance {
    wl_output* output = nullptr;
    std::unique_ptr<LayerSurface> surface;
    AnimationManager animations;
    std::unique_ptr<Node> root;
    Image* back = nullptr;
    Image* front = nullptr;
    std::string shown;
  };

  void createInstances();
  void buildScene(Instance& inst);
  void show(Instance& inst);

  WaylandConnection* m_wayland = nullptr;
  RenderContext* m_renderContext = nullptr;
  AsyncTextureCache* m_textures = nullptr;
  WallpaperService* m_wallpapers = nullptr;
  std::vector<std::unique_ptr<Instance>> m_instances;
};
