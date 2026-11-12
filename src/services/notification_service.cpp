#include "services/notification_service.h"

#include "config/config.h"
#include "core/log.h"
#include "dbus/session_bus.h"
#include "render/codecs/image_encoder.h"
#include "system/icon_resolver.h"
#include "util/file_utils.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace {

  constexpr Logger kLog("notifications");
  constexpr const char* kInterface = "org.freedesktop.Notifications";
  constexpr std::uint32_t kReasonDismissed = 2;
  constexpr std::uint32_t kReasonClosedByCall = 3;
  constexpr std::size_t kMaxText = 4096;

  using ImageData = sdbus::Struct<
      std::int32_t, std::int32_t, std::int32_t, bool, std::int32_t, std::int32_t, std::vector<std::uint8_t>>;

  std::string historyPath() { return FileUtils::stateDir() + "/notifications.json"; }
  std::string imageDir() { return FileUtils::stateDir() + "/notification-images"; }

  std::string truncate(const std::string& s) { return s.size() > kMaxText ? s.substr(0, kMaxText) : s; }

  // "file:///x" or "/x" -> "/x"; anything else is not a path.
  std::string localPath(const std::string& value) {
    if (value.starts_with("file://")) {
      return value.substr(7);
    }
    return value.starts_with("/") ? value : std::string();
  }

  std::string stringHint(const std::map<std::string, sdbus::Variant>& hints, const char* key) {
    const auto it = hints.find(key);
    if (it == hints.end()) {
      return {};
    }
    try {
      return it->second.get<std::string>();
    } catch (const sdbus::Error&) {
      return {};
    }
  }

  // image-data hint (raw pixels) -> PNG file next to the history.
  std::string saveImageData(const sdbus::Variant& value, std::uint32_t id) {
    ImageData data;
    try {
      data = value.get<ImageData>();
    } catch (const sdbus::Error&) {
      return {};
    }
    const auto [width, height, stride, hasAlpha, bits, channels, bytes] = data;
    if (width <= 0 || height <= 0 || bits != 8 || (channels != 3 && channels != 4) || stride < width * channels
        || bytes.size() < static_cast<std::size_t>(stride) * static_cast<std::size_t>(height - 1)
                              + static_cast<std::size_t>(width * channels)) {
      return {};
    }
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4);
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        const std::size_t src = static_cast<std::size_t>(y) * static_cast<std::size_t>(stride)
                                + static_cast<std::size_t>(x * channels);
        const std::size_t dst = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width)
                                 + static_cast<std::size_t>(x)) * 4;
        rgba[dst] = bytes[src];
        rgba[dst + 1] = bytes[src + 1];
        rgba[dst + 2] = bytes[src + 2];
        rgba[dst + 3] = (hasAlpha && channels == 4) ? bytes[src + 3] : 255;
      }
    }
    const auto png = encodePng(rgba.data(), width, height);
    if (png.empty()) {
      return {};
    }
    std::error_code ec;
    std::filesystem::create_directories(imageDir(), ec);
    const std::string path = imageDir() + "/" + std::to_string(id) + "-" + std::to_string(std::time(nullptr)) + ".png";
    std::ofstream(path, std::ios::binary).write(reinterpret_cast<const char*>(png.data()),
                                                 static_cast<std::streamsize>(png.size()));
    return path;
  }

} // namespace

NotificationService::NotificationService(SessionBus& bus, ConfigService& config)
    : m_config(config), m_icons(std::make_unique<IconResolver>(true)) {
  // The resolver's theme index is built on demand by whoever owns the theme state; build it now.
  IconResolver::checkThemeChanged();
  load();
  try {
    m_object = sdbus::createObject(bus.connection(), sdbus::ObjectPath{"/org/freedesktop/Notifications"});
    m_object
        ->addVTable(
            sdbus::registerMethod("Notify")
                .withInputParamNames(
                    "app_name", "replaces_id", "app_icon", "summary", "body", "actions", "hints", "expire_timeout"
                )
                .withOutputParamNames("id")
                .implementedAs([this](
                                   const std::string& appName, std::uint32_t replacesId, const std::string& appIcon,
                                   const std::string& summary, const std::string& body,
                                   const std::vector<std::string>& /*actions*/,
                                   const std::map<std::string, sdbus::Variant>& hints, std::int32_t /*timeout*/
                               ) { return onNotify(appName, replacesId, appIcon, summary, body, hints); }),
            sdbus::registerMethod("GetCapabilities").withOutputParamNames("capabilities").implementedAs([] {
              return std::vector<std::string>{"body", "icon-static", "persistence"};
            }),
            sdbus::registerMethod("GetServerInformation")
                .withOutputParamNames("name", "vendor", "version", "spec_version")
                .implementedAs([] {
                  return std::make_tuple(
                      std::string("Dynamic-Larpland"), std::string("Dynamic-Larpland"), std::string(DL_VERSION),
                      std::string("1.2")
                  );
                }),
            sdbus::registerMethod("CloseNotification").withInputParamNames("id").implementedAs([this](std::uint32_t id) {
              emitClosed(id, kReasonClosedByCall);
            }),
            sdbus::registerSignal("NotificationClosed").withParameters<std::uint32_t, std::uint32_t>("id", "reason"),
            sdbus::registerSignal("ActionInvoked").withParameters<std::uint32_t, std::string>("id", "action_key")
        )
        .forInterface(kInterface);
    bus.connection().requestName(sdbus::ServiceName{kInterface});
    m_ownsName = true;
    kLog.info("notification daemon running");
  } catch (const sdbus::Error& e) {
    // Another daemon (mako, dunst, a desktop shell) owns the name: keep the history only.
    kLog.warn("not the notification daemon: {}", e.what());
  }
}

NotificationService::~NotificationService() = default;

std::uint32_t NotificationService::onNotify(
    const std::string& appName, std::uint32_t replacesId, const std::string& appIcon, const std::string& summary,
    const std::string& body, const std::map<std::string, sdbus::Variant>& hints
) {
  const std::uint32_t id = replacesId != 0 ? replacesId : m_nextId++;
  NotificationEntry entry{
      .id = id,
      .appName = truncate(appName),
      .icon = resolveIcon(id, appIcon, hints),
      .summary = truncate(summary),
      .body = truncate(body),
      .time = std::time(nullptr),
  };
  std::erase_if(m_history, [id](const NotificationEntry& e) { return e.id == id; });
  m_history.insert(m_history.begin(), entry);
  const auto limit = static_cast<std::size_t>(std::max(1, m_config.config().notifications.historyLimit));
  if (m_history.size() > limit) {
    m_history.resize(limit);
  }
  save();
  if (m_onPosted) {
    m_onPosted(entry);
  }
  changed();
  return id;
}

std::string NotificationService::resolveIcon(
    std::uint32_t id, const std::string& appIcon, const std::map<std::string, sdbus::Variant>& hints
) {
  for (const char* key : {"image-data", "image_data", "icon_data"}) {
    if (const auto it = hints.find(key); it != hints.end()) {
      if (std::string path = saveImageData(it->second, id); !path.empty()) {
        return path;
      }
    }
  }
  for (const std::string& candidate : {stringHint(hints, "image-path"), stringHint(hints, "image_path"), appIcon,
                                       stringHint(hints, "desktop-entry")}) {
    if (candidate.empty()) {
      continue;
    }
    if (std::string path = localPath(candidate); !path.empty()) {
      return path;
    }
    if (const std::string& resolved = m_icons->resolve(candidate, 64); !resolved.empty()) {
      return resolved;
    }
  }
  return {};
}

void NotificationService::emitClosed(std::uint32_t id, std::uint32_t reason) {
  if (m_object) {
    m_object->emitSignal("NotificationClosed").onInterface(kInterface).withArguments(id, reason);
  }
}

void NotificationService::dismiss(std::uint32_t id) {
  std::erase_if(m_history, [id](const NotificationEntry& e) { return e.id == id; });
  emitClosed(id, kReasonDismissed);
  save();
  changed();
}

void NotificationService::clear() {
  for (const auto& entry : m_history) {
    emitClosed(entry.id, kReasonDismissed);
  }
  m_history.clear();
  std::error_code ec;
  std::filesystem::remove_all(imageDir(), ec);
  save();
  changed();
}

void NotificationService::load() {
  std::ifstream in(historyPath());
  if (!in) {
    return;
  }
  try {
    for (const auto& item : nlohmann::json::parse(in)) {
      NotificationEntry entry{
          .id = item.value("id", 0U),
          .appName = item.value("app", ""),
          .icon = item.value("icon", ""),
          .summary = item.value("summary", ""),
          .body = item.value("body", ""),
          .time = item.value("time", std::time_t{0}),
      };
      m_nextId = std::max(m_nextId, entry.id + 1);
      m_history.push_back(std::move(entry));
    }
  } catch (const nlohmann::json::exception& e) {
    kLog.warn("{}: {}", historyPath(), e.what());
  }
}

void NotificationService::save() {
  nlohmann::json list = nlohmann::json::array();
  for (const auto& e : m_history) {
    list.push_back({{"id", e.id}, {"app", e.appName}, {"icon", e.icon}, {"summary", e.summary}, {"body", e.body},
                    {"time", e.time}});
  }
  std::error_code ec;
  std::filesystem::create_directories(FileUtils::stateDir(), ec);
  // Write to a temp file and rename, so a crash never leaves a truncated history.
  const std::string tmp = historyPath() + ".tmp";
  std::ofstream(tmp, std::ios::trunc) << list.dump(2) << "\n";
  std::filesystem::rename(tmp, historyPath(), ec);
}

void NotificationService::changed() {
  if (m_onChange) {
    m_onChange();
  }
}
