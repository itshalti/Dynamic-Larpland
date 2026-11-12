#pragma once

#include "wayland/surface.h"
#include "wayland/wayland_extension.h"

#include <functional>

struct ext_session_lock_manager_v1;
struct ext_session_lock_v1;
struct ext_session_lock_surface_v1;
struct wl_output;

// Binds ext_session_lock_manager_v1 (the lock screen protocol).
class SessionLockManager final : public WaylandExtension {
public:
  explicit SessionLockManager(WaylandConnection& connection);

  void onGlobal(wl_registry* registry, std::uint32_t name, std::string_view interface, std::uint32_t version) override;
  void releaseGlobals() override;
  ext_session_lock_manager_v1* manager() const noexcept { return m_manager; }

private:
  ext_session_lock_manager_v1* m_manager = nullptr;
};

// One lock surface per output; the compositor sizes it to the whole output.
class LockSurface final : public Surface {
public:
  explicit LockSurface(WaylandConnection& connection);
  ~LockSurface() override;

  bool initialize() override { return false; }
  bool initialize(ext_session_lock_v1* lock, wl_output* output);

  static void handleConfigure(
      void* data, ext_session_lock_surface_v1* surface, std::uint32_t serial, std::uint32_t width, std::uint32_t height
  );

private:
  ext_session_lock_surface_v1* m_lockSurface = nullptr;
};
