#include "services/idle_service.h"

#include "compositors/hyprland.h"
#include "config/config.h"
#include "core/log.h"
#include "ext-idle-notify-v1-client-protocol.h"
#include "system/process.h"
#include "wayland/wayland_connection.h"

#include <algorithm>

namespace {

  constexpr Logger kLog("idle");

  const ext_idle_notification_v1_listener kListener = {
      .idled = &IdleService::handleIdled,
      .resumed = &IdleService::handleResumed,
  };

} // namespace

IdleNotifier::IdleNotifier(WaylandConnection& /*connection*/) {}

void IdleNotifier::onGlobal(wl_registry* registry, std::uint32_t name, std::string_view interface,
                            std::uint32_t version) {
  if (interface == ext_idle_notifier_v1_interface.name) {
    m_notifier = static_cast<ext_idle_notifier_v1*>(
        wl_registry_bind(registry, name, &ext_idle_notifier_v1_interface, std::min(version, 1U))
    );
  }
}

void IdleNotifier::releaseGlobals() {
  if (m_notifier != nullptr) {
    ext_idle_notifier_v1_destroy(m_notifier);
    m_notifier = nullptr;
  }
}

IdleService::IdleService(WaylandConnection& wayland, ConfigService& config, Actions actions)
    : m_wayland(wayland), m_config(config), m_actions(std::move(actions)) {
  reload();
}

IdleService::~IdleService() { clear(); }

void IdleService::clear() {
  for (auto& behavior : m_behaviors) {
    if (behavior->idled) {
      resume(*behavior);
    }
    if (behavior->notification != nullptr) {
      ext_idle_notification_v1_destroy(behavior->notification);
    }
  }
  m_behaviors.clear();
}

void IdleService::reload() {
  clear();
  auto* notifier = m_wayland.extension<IdleNotifier>().notifier();
  if (notifier == nullptr || m_wayland.seat() == nullptr) {
    kLog.warn("the compositor has no ext-idle-notify-v1; idle behaviors are off");
    return;
  }
  const auto& cfg = m_config.config().idle;
  const std::pair<const char*, std::pair<bool, int>> behaviors[] = {
      {"lock", {cfg.lockEnabled, cfg.lockTimeoutS}},
      {"screen_off", {cfg.screenOffEnabled, cfg.screenOffTimeoutS}},
      {"suspend", {cfg.suspendEnabled, cfg.suspendTimeoutS}},
  };
  for (const auto& [action, setting] : behaviors) {
    const auto [enabled, seconds] = setting;
    if (!enabled || seconds <= 0) {
      continue;
    }
    auto behavior = std::make_unique<Behavior>();
    behavior->owner = this;
    behavior->action = action;
    behavior->notification = ext_idle_notifier_v1_get_idle_notification(
        notifier, static_cast<std::uint32_t>(seconds) * 1000U, m_wayland.seat()
    );
    ext_idle_notification_v1_add_listener(behavior->notification, &kListener, behavior.get());
    kLog.info("idle behavior {} after {} s", action, seconds);
    m_behaviors.push_back(std::move(behavior));
  }
}

void IdleService::handleIdled(void* data, ext_idle_notification_v1* /*notification*/) {
  auto* behavior = static_cast<Behavior*>(data);
  behavior->owner->run(*behavior);
}

void IdleService::handleResumed(void* data, ext_idle_notification_v1* /*notification*/) {
  auto* behavior = static_cast<Behavior*>(data);
  if (behavior->idled) {
    behavior->owner->resume(*behavior);
  }
}

void IdleService::run(Behavior& behavior) {
  if (m_config.config().idle.inhibitWhilePlaying && m_actions.mediaPlaying && m_actions.mediaPlaying()) {
    kLog.debug("idle {} skipped: media is playing", behavior.action);
    return;
  }
  behavior.idled = true;
  kLog.info("idle: {}", behavior.action);
  if (behavior.action == "lock") {
    m_actions.lock();
  } else if (behavior.action == "screen_off") {
    if (!HyprlandService::available() || !HyprlandService::setDpms(false)) {
      kLog.warn("screen off is only supported on Hyprland");
    }
  } else if (behavior.action == "suspend") {
    m_actions.lock();
    if (!process::runAsync({"systemctl", "suspend"})) {
      kLog.warn("could not run systemctl suspend");
    }
  }
}

void IdleService::resume(Behavior& behavior) {
  behavior.idled = false;
  if (behavior.action == "screen_off" && HyprlandService::available()) {
    HyprlandService::setDpms(true);
  }
}
