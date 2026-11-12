#pragma once

#include <cstdint>
#include <ctime>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <sdbus-c++/sdbus-c++.h>

class ConfigService;
class IconResolver;
class SessionBus;

struct NotificationEntry {
  std::uint32_t id = 0;
  std::string appName;
  // Local image file for the app icon / notification image, or "".
  std::string icon;
  std::string summary;
  std::string body;
  std::time_t time = 0;
};

// The island is the notification daemon (org.freedesktop.Notifications on the session bus). Notifications
// are kept in a newest-first history persisted to $XDG_STATE_HOME/dynamic-larpland/notifications.json.
class NotificationService {
public:
  using PostedCallback = std::function<void(const NotificationEntry&)>;
  using ChangeCallback = std::function<void()>;

  NotificationService(SessionBus& bus, ConfigService& config);
  ~NotificationService();

  bool ownsName() const noexcept { return m_ownsName; }
  const std::vector<NotificationEntry>& history() const noexcept { return m_history; }
  void dismiss(std::uint32_t id);
  void clear();

  // A new (or replacing) notification arrived: shown as a live activity.
  void setPostedCallback(PostedCallback callback) { m_onPosted = std::move(callback); }
  // History changed.
  void setChangeCallback(ChangeCallback callback) { m_onChange = std::move(callback); }

private:
  std::uint32_t onNotify(
      const std::string& appName, std::uint32_t replacesId, const std::string& appIcon, const std::string& summary,
      const std::string& body, const std::map<std::string, sdbus::Variant>& hints
  );
  std::string resolveIcon(
      std::uint32_t id, const std::string& appIcon, const std::map<std::string, sdbus::Variant>& hints
  );
  void emitClosed(std::uint32_t id, std::uint32_t reason);
  void load();
  void save();
  void changed();

  ConfigService& m_config;
  std::unique_ptr<sdbus::IObject> m_object;
  std::unique_ptr<IconResolver> m_icons;
  std::vector<NotificationEntry> m_history;
  std::uint32_t m_nextId = 1;
  bool m_ownsName = false;
  PostedCallback m_onPosted;
  ChangeCallback m_onChange;
};
