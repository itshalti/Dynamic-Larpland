#pragma once

#include "core/timer_manager.h"
#include "island/calendar_pages.h"
#include "island/page_context.h"
#include "island/theme.h"
#include "services/notification_service.h"
#include "render/animation/animation_manager.h"
#include "render/scene/input_dispatcher.h"
#include "render/scene/node.h"
#include "wayland/layer_surface.h"

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class AsyncTextureCache;
class Box;
class ConfigService;
class ContextMenuPopup;
class IconResolver;
class Label;
class TrayService;
class MediaService;
class NotificationService;
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
  Islands();
  ~Islands();

  // Connector name of the focused monitor ("" = unknown).
  void setFocusedMonitorProvider(std::function<std::string()> provider) { m_focusedMonitor = std::move(provider); }

  void initialize(
      WaylandConnection& wayland, RenderContext& renderContext, ConfigService& config, AsyncTextureCache& textures,
      WallpaperService& wallpapers, MediaService& media, NotificationService& notifications, TrayService& tray,
      IconResolver& icons
  );
  // Handles actions pages trigger that aren't island modes ("record", "settings").
  void setActionHandler(std::function<void(const std::string&)> handler) { m_actionHandler = std::move(handler); }
  // Re-creates the per-output surfaces after outputs were added or removed.
  void onOutputsChanged();
  void onConfigChanged();
  // Wallpaper list, current wallpaper or palette changed: rebuild the pages that show them.
  void onWallpapersChanged();
  void onMediaChanged();
  void onNotificationsChanged();
  void onTrayChanged();
  // A new notification: shown briefly as a live activity on collapsed islands.
  void onNotificationPosted(const NotificationEntry& entry);
  bool onPointerEvent(const PointerEvent& event);
  bool onKeyboardEvent(const KeyboardEvent& event);

  // Actions from IPC/keybinds act on the island of the focused monitor (Hyprland); when it is unknown
  // (other compositors), on every island.
  void toggle();
  void setMode(const std::string& mode);
  // Opens `mode`, or collapses if it is already open.
  void toggleMode(const std::string& mode);

private:
  struct Instance {
    wl_output* output = nullptr;
    std::string connector;
    std::unique_ptr<LayerSurface> surface;
    AnimationManager animations;
    InputDispatcher input;
    std::unique_ptr<Node> root;
    Box* pill = nullptr;
    std::optional<NotificationEntry> popup;
    Timer popupTimer;
    std::unordered_map<std::string, Node*> pages;
    std::unordered_map<std::string, std::function<void()>> ticks;
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
  // Instances an IPC/keybind action applies to.
  std::vector<Instance*> targets();
  void buildScene(Instance& inst);
  BuiltPage buildPage(Instance& inst, const std::string& mode);
  void runAction(Instance& inst, const std::string& action);
  void openTrayMenu(Instance& inst, const std::string& itemId, float x, float y, float w, float h,
                    std::int32_t parentId = 0);
  void rebuildPage(Instance& inst, const std::string& mode);
  void applyGeometry(Instance& inst);
  theme::Size targetSize(const Instance& inst, const std::string& mode) const;
  // Animates the pill to the target size of the current mode.
  void morphTo(Instance& inst);
  // Rebuilds the collapsed content and resizes the pill if it is collapsed.
  void refreshCollapsed(Instance& inst);
  void setMode(Instance& inst, const std::string& mode);
  void scheduleClose(Instance& inst);
  void updateClock();

  WaylandConnection* m_wayland = nullptr;
  RenderContext* m_renderContext = nullptr;
  ConfigService* m_config = nullptr;
  AsyncTextureCache* m_textures = nullptr;
  WallpaperService* m_wallpapers = nullptr;
  MediaService* m_media = nullptr;
  NotificationService* m_notifications = nullptr;
  TrayService* m_tray = nullptr;
  IconResolver* m_icons = nullptr;
  // Tray item menu (xdg_popup on the island surface); the island stays open while it is shown.
  std::unique_ptr<ContextMenuPopup> m_trayMenu;
  bool m_menuOpen = false;
  // A menu whose entries were still loading over D-Bus: opened when the tray reports a change.
  struct PendingMenu {
    Instance* inst;
    std::string itemId;
    float x, y, w, h;
    std::int32_t parentId;
    std::chrono::steady_clock::time_point requestedAt;
  };
  std::optional<PendingMenu> m_pendingMenu;
  std::function<void(const std::string&)> m_actionHandler;
  CalendarState m_calendar;
  std::function<std::string()> m_focusedMonitor;
  Timer m_tickTimer;
  std::vector<std::unique_ptr<Instance>> m_instances;
  std::string m_time;
  Timer m_clockTimer;
};
