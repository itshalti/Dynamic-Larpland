#pragma once

#include "render/animation/animation_manager.h"
#include "render/scene/input_dispatcher.h"
#include "render/scene/node.h"
#include "wayland/layer_surface.h"

#include <memory>
#include <string>

class ConfigService;
class Flex;
class Input;
class NotificationService;
class RenderContext;
class SelectDropdownPopup;
class WallpaperService;
class WaylandConnection;
struct KeyboardEvent;
struct PointerEvent;

namespace settings {
  struct Setting;
}

// Settings window: a centered island-style card (own layer surface, so it is centered on any compositor) with
// a sidebar of sections and the selected page on the right. The content comes from settings::registry(); the
// header has Noctalia's search, Advanced and Overridden filters and Reset page. Every change is written to
// config.toml at once. Esc or the close button closes it.
class SettingsWindow {
public:
  SettingsWindow();
  ~SettingsWindow();

  void initialize(WaylandConnection& wayland, RenderContext& renderContext, ConfigService& config,
                  WallpaperService& wallpapers, NotificationService& notifications);

  bool isOpen() const noexcept { return m_surface != nullptr; }
  void toggle(const std::string& connector);
  // Opens (or switches) to a section by id ("notifications").
  void openSection(const std::string& connector, const std::string& section);
  void open(const std::string& connector);
  void close();

  bool onPointerEvent(const PointerEvent& event);
  // Keyboard goes here while the window has keyboard focus.
  bool onKeyboardEvent(const KeyboardEvent& event);
  bool ownsSurface(const struct wl_surface* surface) const noexcept;
  // Data shown on a page changed elsewhere (e.g. the community catalog finished downloading).
  void refresh() { requestRefresh(); }

private:
  void buildScene();
  void buildHeader(Flex& column);
  // Rebuilds the sidebar and the page from the current section / search / filters (the header stays).
  void refreshContent();
  void buildSidebar();
  std::unique_ptr<Node> buildPage();
  bool visible(const settings::Setting& setting) const;
  void buildRow(Flex& card, const settings::Setting& setting);
  void buildCustom(Flex& card, const settings::Setting& setting);
  void afterChange(const std::string& path);
  void resetPage();
  void requestRefresh();

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
  Flex* m_sidebar = nullptr;
  Flex* m_pageHost = nullptr;
  Input* m_search = nullptr;

  std::size_t m_section = 0;
  std::string m_query;
  bool m_showAdvanced = false;
  bool m_onlyOverridden = false;
  // Keybind being recorded ("" = none).
  std::string m_recording;
  bool m_refreshPending = false;
};
