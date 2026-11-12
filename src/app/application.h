#pragma once

#include "app/deferred_call_poll_source.h"
#include "app/timer_poll_source.h"
#include "compositors/hyprland.h"
#include "config/config.h"
#include "dbus/session_bus.h"
#include "dbus/session_bus_poll_source.h"
#include "dbus/tray_service.h"
#include "system/icon_resolver.h"
#include "net/http_client.h"
#include "net/http_client_poll_source.h"
#include "services/media_service.h"
#include "services/notification_service.h"
#include "services/recorder_service.h"
#include "ipc/ipc.h"
#include "island/island.h"
#include "launcher/launcher.h"
#include "lock/lock_screen.h"
#include "settings/settings_window.h"
#include "services/wallpaper_service.h"
#include "wallpaper/background.h"
#include "render/core/async_texture_cache.h"
#include "render/gl_shared_context.h"
#include "render/render_context.h"
#include "wayland/wayland_connection.h"

#include <string>

// Owns the Wayland connection, the GL context and every service/UI part, and runs the main loop.
class Application {
public:
  Application();
  ~Application();

  void run();

private:
  std::string handleAction(const std::string& action);
  void runSystemAction(const std::string& what);

  WaylandConnection m_wayland;
  GlSharedContext m_glShared;
  RenderContext m_renderContext;
  ConfigService m_config;
  WallpaperService m_wallpapers{m_config};
  SessionBus m_sessionBus;
  SessionBusPollSource m_sessionBusSource{m_sessionBus};
  HttpClient m_http;
  HttpClientPollSource m_httpSource{m_http};
  MediaService m_media{m_sessionBus, m_http};
  NotificationService m_notifications{m_sessionBus, m_config};
  TrayService m_tray{m_sessionBus};
  IconResolver m_icons{true};
  RecorderService m_recorder{m_config};
  AsyncTextureCache m_textures;
  HyprlandService m_hyprland;
  TimerPollSource m_timers;
  DeferredCallPollSource m_deferred;
  IpcServer m_ipc;
  Backgrounds m_backgrounds;
  Islands m_islands;
  Launcher m_launcher;
  LockScreen m_lockScreen;
  SettingsWindow m_settings;
  struct wl_surface* m_keyboardFocus = nullptr;
};
