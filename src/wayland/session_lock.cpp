#include "wayland/session_lock.h"

#include "ext-session-lock-v1-client-protocol.h"
#include "wayland/wayland_connection.h"

#include <algorithm>

namespace {

  const ext_session_lock_surface_v1_listener kLockSurfaceListener = {
      .configure = &LockSurface::handleConfigure,
  };

} // namespace

SessionLockManager::SessionLockManager(WaylandConnection& /*connection*/) {}

void SessionLockManager::onGlobal(
    wl_registry* registry, std::uint32_t name, std::string_view interface, std::uint32_t version
) {
  if (interface == ext_session_lock_manager_v1_interface.name) {
    m_manager = static_cast<ext_session_lock_manager_v1*>(
        wl_registry_bind(registry, name, &ext_session_lock_manager_v1_interface, std::min(version, 1U))
    );
  }
}

void SessionLockManager::releaseGlobals() {
  if (m_manager != nullptr) {
    ext_session_lock_manager_v1_destroy(m_manager);
    m_manager = nullptr;
  }
}

LockSurface::LockSurface(WaylandConnection& connection) : Surface(connection) { setDebugName("lock"); }

LockSurface::~LockSurface() {
  m_connection.unregisterSurface(m_surface);
  if (m_lockSurface != nullptr) {
    ext_session_lock_surface_v1_destroy(m_lockSurface);
    m_lockSurface = nullptr;
  }
}

bool LockSurface::initialize(ext_session_lock_v1* lock, wl_output* output) {
  if (lock == nullptr || output == nullptr) {
    return false;
  }
  if (const auto* wlOutput = m_connection.findOutputByWl(output); wlOutput != nullptr) {
    setBufferScale(wlOutput->scale);
  }
  if (!createWlSurface()) {
    return false;
  }
  m_connection.registerSurfaceOutput(m_surface, output);
  m_lockSurface = ext_session_lock_v1_get_lock_surface(lock, m_surface, output);
  ext_session_lock_surface_v1_add_listener(m_lockSurface, &kLockSurfaceListener, this);
  setRunning(true);
  return true;
}

void LockSurface::handleConfigure(
    void* data, ext_session_lock_surface_v1* surface, std::uint32_t serial, std::uint32_t width, std::uint32_t height
) {
  auto* self = static_cast<LockSurface*>(data);
  ext_session_lock_surface_v1_ack_configure(surface, serial);
  self->onConfigure(width, height);
}
