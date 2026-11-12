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

  m_islands.initialize(m_wayland, m_renderContext);
  m_wayland.setOutputChangeCallback([this] { m_islands.onOutputsChanged(); });
  m_wayland.setPointerEventCallback([this](const PointerEvent& event) { m_islands.onPointerEvent(event); });

  m_ipc.start([this](const std::string& action) { return handleAction(action); });
}

Application::~Application() = default;

std::string Application::handleAction(const std::string& action) {
  if (action == "toggle") {
    m_islands.toggle();
  } else if (action == "expand") {
    m_islands.setExpanded(true);
  } else if (action == "collapse") {
    m_islands.setExpanded(false);
  } else if (action == "quit") {
    MainLoop::requestQuit();
  } else {
    return "unknown action: " + action;
  }
  return "ok";
}

void Application::run() {
  MainLoop loop(m_wayland, [this] { return std::vector<PollSource*>{&m_timers, &m_deferred, &m_ipc}; });
  kLog.info("running");
  loop.run();
}
