#pragma once

#include "render/animation/animation_manager.h"
#include "render/scene/input_dispatcher.h"
#include "render/scene/node.h"
#include "wayland/layer_surface.h"

#include <memory>
#include <string>
#include <vector>

class AsyncTextureCache;
class IconResolver;
class Label;
class RenderContext;
class WallpaperService;
class WaylandConnection;
struct DesktopEntry;
struct KeyboardEvent;
struct PointerEvent;
struct wl_output;

// App launcher (default Super+Space): a full-screen transparent overlay on the focused output with a rounded
// black card - search line, fuzzy-filtered apps (icon, name, generic name), up/down, Enter launches, Esc or a
// click outside closes. The selected row is highlighted with an accent bar on the left.
class Launcher {
public:
  void initialize(WaylandConnection& wayland, RenderContext& renderContext, AsyncTextureCache& textures,
                  IconResolver& icons, WallpaperService& wallpapers);

  bool isOpen() const noexcept { return m_surface != nullptr; }
  // connector: output to open on ("" = first output).
  void toggle(const std::string& connector);
  void open(const std::string& connector);
  void close();

  bool onPointerEvent(const PointerEvent& event);
  bool onKeyboardEvent(const KeyboardEvent& event);

private:
  void filter();
  void buildScene();
  void rebuildList();
  void launch(std::size_t index);
  void moveSelection(int delta);

  WaylandConnection* m_wayland = nullptr;
  RenderContext* m_renderContext = nullptr;
  AsyncTextureCache* m_textures = nullptr;
  IconResolver* m_icons = nullptr;
  WallpaperService* m_wallpapers = nullptr;

  std::unique_ptr<LayerSurface> m_surface;
  AnimationManager m_animations;
  InputDispatcher m_input;
  std::unique_ptr<Node> m_root;
  Node* m_card = nullptr;
  Node* m_list = nullptr;
  Label* m_query = nullptr;

  std::string m_text;
  std::vector<const DesktopEntry*> m_results;
  std::size_t m_selected = 0;
  std::size_t m_scroll = 0;
};
