#include "app/application.h"

#include "app/main_loop.h"
#include "core/log.h"
#include "wayland/layer_shell.h"
#include "wayland/wayland_seat.h"

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

  m_islands.initialize(m_wayland, m_renderContext, m_config);
  m_config.setChangeCallback([this] { m_islands.onConfigChanged(); });
  m_wayland.setOutputChangeCallback([this] { m_islands.onOutputsChanged(); });
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
  } else if (action == "quit") {
    MainLoop::requestQuit();
  } else {
    return "unknown action: " + action;
  }
  return "ok";
}

void Application::run() {
  MainLoop loop(m_wayland, [this] { return std::vector<PollSource*>{&m_timers, &m_deferred, &m_ipc, &m_config}; });
  kLog.info("running");
  loop.run();
}
