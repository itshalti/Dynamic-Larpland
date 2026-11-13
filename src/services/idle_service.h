#pragma once

#include "wayland/wayland_extension.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class ConfigService;
class WaylandConnection;
struct ext_idle_notification_v1;
struct ext_idle_notifier_v1;

// Binds ext_idle_notifier_v1.
class IdleNotifier final : public WaylandExtension {
public:
  explicit IdleNotifier(WaylandConnection& connection);
  void onGlobal(wl_registry* registry, std::uint32_t name, std::string_view interface, std::uint32_t version) override;
  void releaseGlobals() override;
  ext_idle_notifier_v1* notifier() const noexcept { return m_notifier; }

private:
  ext_idle_notifier_v1* m_notifier = nullptr;
};

// Idle behaviors (Noctalia's defaults): after idle.*_timeout_s of no input, lock the screen, turn the screens off
// (back on at the next input) and lock + suspend. Each runs only when enabled, and none runs while media plays
// when idle.inhibit_while_playing is set. The compositor's own idle inhibitors (video players) apply too.
class IdleService {
public:
  struct Actions {
    std::function<void()> lock;
    std::function<bool()> mediaPlaying;
  };

  IdleService(WaylandConnection& wayland, ConfigService& config, Actions actions);
  ~IdleService();

  // Re-creates the idle notifications from the config.
  void reload();

  static void handleIdled(void* data, ext_idle_notification_v1* notification);
  static void handleResumed(void* data, ext_idle_notification_v1* notification);

private:
  struct Behavior {
    IdleService* owner = nullptr;
    std::string action;
    ext_idle_notification_v1* notification = nullptr;
    bool idled = false;
  };

  void run(Behavior& behavior);
  void resume(Behavior& behavior);
  void clear();

  WaylandConnection& m_wayland;
  ConfigService& m_config;
  Actions m_actions;
  std::vector<std::unique_ptr<Behavior>> m_behaviors;
};
