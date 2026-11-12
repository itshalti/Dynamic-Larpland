#include "app/application.h"

#include "app/main_loop.h"
#include "island/ui_palette.h"
#include "core/log.h"
#include "render/backend/render_backend.h"
#include "wayland/layer_shell.h"
#include "wayland/wayland_seat.h"

#include <filesystem>
#include <stdexcept>

namespace {

  constexpr Logger kLog("app");

} // namespace

Application::Application() {
  // Extensions must exist before connect() so the registry roundtrip binds their globals.
  m_wayland.extension<LayerShell>();
  if (!m_wayland.connect()) {
    throw std::runtime_error("failed to connect to the Wayland display");
  }
  if (m_wayland.extension<LayerShell>().manager() == nullptr) {
    throw std::runtime_error("the compositor does not support wlr-layer-shell");
  }

  m_glShared.initialize(m_wayland.display());
  m_renderContext.initialize(m_glShared);
  m_textures.initialize(m_glShared.hasSharedContext() ? &m_glShared : nullptr);
  if (!m_glShared.hasSharedContext()) {
    m_textures.setMakeCurrentCallback([this] { m_renderContext.backend().makeCurrentNoSurface(); });
  }

  applyUiPalette(m_wallpapers);
  // Created before the islands so the wallpaper sits below them in the same compositor pass.
  m_backgrounds.initialize(m_wayland, m_renderContext, m_textures, m_wallpapers);
  m_islands.initialize(
      m_wayland, m_renderContext, m_config, m_textures, m_wallpapers, m_media, m_notifications, m_tray, m_icons
  );
  m_tray.setChangeCallback([this] { m_islands.onTrayChanged(); });
  m_tray.start();
  m_notifications.setChangeCallback([this] { m_islands.onNotificationsChanged(); });
  m_notifications.setPostedCallback([this](const NotificationEntry& entry) { m_islands.onNotificationPosted(entry); });
  m_islands.setFocusedMonitorProvider([this] { return m_hyprland.focusedMonitor(); });
  m_islands.setActionHandler([this](const std::string& action) { handleAction(action); });
  m_media.setChangeCallback([this] { m_islands.onMediaChanged(); });
  m_wallpapers.setChangeCallback([this] {
    applyUiPalette(m_wallpapers);
    m_backgrounds.refresh();
    m_islands.onWallpapersChanged();
  });
  m_config.setChangeCallback([this] {
    m_islands.onConfigChanged();
    m_wallpapers.onConfigChanged();
  });
  m_wayland.setOutputChangeCallback([this] {
    m_backgrounds.onOutputsChanged();
    m_islands.onOutputsChanged();
  });
  m_wayland.setPointerEventCallback([this](const PointerEvent& event) { m_islands.onPointerEvent(event); });
  m_wayland.setKeyboardEventCallback([this](const KeyboardEvent& event) { m_islands.onKeyboardEvent(event); });

  m_ipc.start([this](const std::string& action) { return handleAction(action); });
}

Application::~Application() = default;

std::string Application::handleAction(const std::string& action) {
  if (action == "toggle") {
    m_islands.toggle();
  } else if (action == "expand") {
    m_islands.setMode("expanded");
  } else if (action == "collapse") {
    m_islands.setMode("collapsed");
  } else if (action == "player" || action == "calendar" || action == "wallpapers" || action == "power") {
    m_islands.toggleMode(action);
  } else if (action.starts_with("wallpaper ")) {
    const std::string path = action.substr(10);
    if (!std::filesystem::is_regular_file(path)) {
      return "no such file: " + path;
    }
    m_wallpapers.set(path);
  } else if (action == "quit") {
    MainLoop::requestQuit();
  } else {
    return "unknown action: " + action;
  }
  return "ok";
}

void Application::run() {
  MainLoop loop(m_wayland, [this] { return std::vector<PollSource*>{&m_timers, &m_deferred, &m_ipc, &m_config, &m_textures, &m_sessionBusSource, &m_httpSource, &m_hyprland}; });
  kLog.info("running");
  loop.run();
}
