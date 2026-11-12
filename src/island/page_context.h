#pragma once

#include <functional>

class AnimationManager;
class AsyncTextureCache;
class Renderer;
class WallpaperService;

// What an island page needs to build itself and to talk to the services.
struct PageContext {
  Renderer& renderer;
  AnimationManager& animations;
  AsyncTextureCache& textures;
  WallpaperService& wallpapers;
  std::function<void()> requestRedraw;
};
