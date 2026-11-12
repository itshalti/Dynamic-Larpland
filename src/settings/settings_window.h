#pragma once

#include "render/animation/animation_manager.h"
#include "render/scene/input_dispatcher.h"
#include "render/scene/node.h"
#include "wayland/layer_surface.h"

#include <memory>
#include <string>

class ConfigService;
class Flex;
class NotificationService;
class RenderContext;
class SelectDropdownPopup;
class WallpaperService;
class WaylandConnection;
struct KeyboardEvent;
struct PointerEvent;

// Settings window: a centered island-style card (own layer surface, so it is centered on any compositor) with
// a sidebar of sections and the selected page on the right. Every change is written to config.toml at once.
// Esc or the close button closes it.
class SettingsWindow {
public:
  SettingsWindow();
  ~SettingsWindow();

  void initialize(WaylandConnection& wayland, RenderContext& renderContext, ConfigService& config,
                  WallpaperService& wallpapers, NotificationService& notifications);

  bool isOpen() const noexcept { return m_surface != nullptr; }
  void toggle(const std::string& connector);
  void open(const std::string& connector);
  void close();

  bool onPointerEvent(const PointerEvent& event);
  // Keyboard goes here while the window has keyboard focus.
  bool onKeyboardEvent(const KeyboardEvent& event);
  bool ownsSurface(const struct wl_surface* surface) const noexcept;

private:
  void buildScene();
  void showPage(std::size_t index);
  std::unique_ptr<Node> buildPage(std::size_t index);
  void buildLookPage(Flex& page);
  void buildIslandPage(Flex& page);
  void buildNotificationsPage(Flex& page);
  void buildSystemPage(Flex& page);
  void requestRebuild();
  void save();

  WaylandConnection* m_wayland = nullptr;
  RenderContext* m_renderContext = nullptr;
  ConfigService* m_config = nullptr;
  WallpaperService* m_wallpapers = nullptr;
  NotificationService* m_notifications = nullptr;

  std::unique_ptr<LayerSurface> m_surface;
  std::unique_ptr<SelectDropdownPopup> m_dropdown;
  AnimationManager m_animations;
  InputDispatcher m_input;
  std::unique_ptr<Node> m_root;
  Node* m_pageHost = nullptr;
  std::size_t m_page = 0;
  // Keybind being recorded ("" = none).
  std::string m_recording;
  bool m_rebuildPending = false;
};
