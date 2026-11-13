#include "app/application.h"

#include "app/main_loop.h"
#include "compositors/keybinds.h"
#include "island/ui_palette.h"
#include "system/day_night_schedule.h"
#include "system/process.h"
#include "core/log.h"
#include "render/backend/render_backend.h"
#include "wayland/layer_shell.h"
#include "wayland/session_lock.h"
#include "wayland/wayland_seat.h"

#include <filesystem>
#include <stdexcept>

namespace {

  constexpr Logger kLog("app");

} // namespace

Application::Application() {
  // Extensions must exist before connect() so the registry roundtrip binds their globals.
  m_wayland.extension<LayerShell>();
  m_wayland.extension<SessionLockManager>();
  m_wayland.extension<IdleNotifier>();
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
      m_wayland, m_renderContext, m_config, m_textures, m_wallpapers, m_media, m_notifications, m_tray, m_icons,
      m_recorder
  );
  m_recorder.setChangeCallback([this] { m_islands.onRecorderChanged(); });
  m_tray.setChangeCallback([this] { m_islands.onTrayChanged(); });
  m_tray.start();
  m_notifications.setChangeCallback([this] { m_islands.onNotificationsChanged(); });
  m_notifications.setPostedCallback([this](const NotificationEntry& entry) {
    if (!(m_lockScreen.active() && m_config.config().lockscreen.dnd)) {
      m_islands.onNotificationPosted(entry);
    }
  });
  m_islands.setFocusedMonitorProvider([this] { return m_hyprland.focusedMonitor(); });
  // Page actions: the power ones exist only here (behind the page's confirm), never as IPC actions.
  m_islands.setActionHandler([this](const std::string& action) {
    if (action.starts_with("system:")) {
      runSystemAction(action.substr(7));
    } else {
      handleAction(action);
    }
  });
  m_media.setChangeCallback([this] { m_islands.onMediaChanged(); });
  m_wallpapers.setChangeCallback([this] {
    applyUiPalette(m_wallpapers);
    m_backgrounds.refresh();
    m_islands.onWallpapersChanged();
  });
  keybinds::writeHyprland(m_config.config());
  m_idle = std::make_unique<IdleService>(
      m_wayland, m_config,
      IdleService::Actions{
          .lock = [this] { handleAction("lock"); },
          .mediaPlaying = [this] { return m_media.playing(); },
      }
  );
  m_communityTemplates.setReadyCallback([this] { m_settings.refresh(); });
  syncCommunityTemplates();
  syncDayNight();
  m_dayNightTimer.startRepeating(std::chrono::minutes(1), [this] { syncDayNight(); });
  m_config.setChangeCallback([this] {
    keybinds::writeHyprland(m_config.config());
    m_idle->reload();
    syncCommunityTemplates();
    syncDayNight();
    m_islands.onConfigChanged();
    m_wallpapers.onConfigChanged();
  });
  m_wayland.setOutputChangeCallback([this] {
    m_backgrounds.onOutputsChanged();
    m_islands.onOutputsChanged();
  });
  m_launcher.initialize(m_wayland, m_renderContext, m_textures, m_icons, m_wallpapers);
  m_lockScreen.initialize(m_wayland, m_renderContext, m_textures, m_wallpapers, m_config, &m_systemBus);
  m_settings.initialize(m_wayland, m_renderContext, m_config, m_wallpapers, m_notifications);
  m_wayland.setKeyboardFocusCallback([this](wl_surface* surface, bool entered) {
    m_keyboardFocus = entered ? surface : (m_keyboardFocus == surface ? nullptr : m_keyboardFocus);
  });
  // The lock screen, then the launcher (both grab the keyboard), get input first.
  m_wayland.setPointerEventCallback([this](const PointerEvent& event) {
    if (!m_lockScreen.onPointerEvent(event) && !m_launcher.onPointerEvent(event)
        && !m_settings.onPointerEvent(event)) {
      m_islands.onPointerEvent(event);
    }
  });
  m_wayland.setKeyboardEventCallback([this](const KeyboardEvent& event) {
    if (m_lockScreen.onKeyboardEvent(event) || m_launcher.onKeyboardEvent(event)) {
      return;
    }
    if (m_settings.ownsSurface(m_keyboardFocus) && m_settings.onKeyboardEvent(event)) {
      return;
    }
    m_islands.onKeyboardEvent(event);
  });

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
  } else if (action == "lock") {
    m_launcher.close();
    m_lockScreen.lock(m_islands.targetConnector());
  } else if (action == "dnd") {
    m_config.set("notifications.dnd", toml::value<bool>(!m_config.config().notifications.dnd));
  } else if (action.starts_with("settings ")) {
    m_settings.openSection(m_islands.targetConnector(), action.substr(9));
  } else if (action == "settings") {
    m_settings.toggle(m_islands.targetConnector());
  } else if (action == "launcher") {
    m_launcher.toggle(m_islands.targetConnector());
  } else if (action == "record") {
    m_recorder.toggle(m_islands.targetConnector());
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

void Application::syncCommunityTemplates() {
  // Downloads the catalog and the selected templates into the cache (a no-op when switched off).
  const auto& theme = m_config.config().theme;
  m_communityTemplates.sync({.enableCommunityTemplates = theme.communityTemplates, .communityIds = theme.community});
}

void Application::syncDayNight() {
  const auto& cfg = m_config.config();
  const bool configured = cfg.location.customSchedule || (cfg.location.latitude && cfg.location.longitude);
  if (!cfg.theme.autoMode || !configured) {
    return;
  }
  const bool night = day_night_schedule::evaluate(cfg.location, std::nullopt, std::nullopt).night;
  const std::string mode = night ? "dark" : "light";
  if (cfg.theme.mode == mode) {
    return;
  }
  kLog.info("switching to {} mode ({})", mode, night ? "night" : "day");
  m_config.set("theme.mode", toml::value<std::string>(mode));
  if (!m_wallpapers.current().empty()) {
    m_wallpapers.set(m_wallpapers.current());
  }
}

void Application::runSystemAction(const std::string& what) {
  std::vector<std::string> command;
  if (what == "poweroff") {
    command = {"systemctl", "poweroff"};
  } else if (what == "reboot") {
    command = {"systemctl", "reboot"};
  } else if (what == "firmware") {
    command = {"systemctl", "reboot", "--firmware-setup"};
  } else if (what == "suspend") {
    command = {"systemctl", "suspend"};
  } else {
    return;
  }
  if (!process::runAsync(command)) {
    kLog.warn("could not run {}", command.front());
  }
}

void Application::run() {
  MainLoop loop(m_wayland, [this] { return std::vector<PollSource*>{&m_timers, &m_deferred, &m_ipc, &m_config, &m_textures, &m_sessionBusSource, &m_systemBusSource, &m_httpSource, &m_hyprland}; });
  kLog.info("running");
  loop.run();
}
