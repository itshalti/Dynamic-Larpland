#include "config/config.h"

#include "core/log.h"
#include "util/file_utils.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ranges>

namespace {

  constexpr Logger kLog("config");
  constexpr const char* kFileName = "config.toml";

  toml::table toTable(const Config& c) {
    toml::array templates;
    for (const auto& id : c.theme.templates) {
      templates.push_back(id);
    }
    toml::array community;
    for (const auto& id : c.theme.community) {
      community.push_back(id);
    }
    toml::array rules;
    for (const auto& rule : c.notifications.rules) {
      rules.push_back(toml::table{{"app", rule.app}, {"action", rule.action}});
    }
    toml::table location{
        {"custom_schedule", c.location.customSchedule},
        {"sunrise", c.location.sunrise},
        {"sunset", c.location.sunset},
    };
    // Unset coordinates stay out of the file.
    if (c.location.latitude) {
      location.insert("latitude", *c.location.latitude);
    }
    if (c.location.longitude) {
      location.insert("longitude", *c.location.longitude);
    }
    toml::table binds;
    for (const auto& [action, keys] : c.keybinds) {
      binds.insert(action, keys);
    }
    return toml::table{
        {"keybinds", binds},
        {"location", location},
        {"island",
         toml::table{
             {"hover_expand", c.island.hoverExpand},
             {"hover_delay_ms", c.island.hoverDelayMs},
             {"time_format", c.island.timeFormat},
         }},
        {"calendar", toml::table{{"first_day_of_week", c.calendar.firstDayOfWeek}}},
        {"lockscreen",
         toml::table{
             {"wallpaper", c.lockscreen.wallpaper},
             {"dnd", c.lockscreen.dnd},
             {"fingerprint", c.lockscreen.fingerprint},
             {"monitors", c.lockscreen.monitors},
         }},
        {"idle",
         toml::table{
             {"lock_enabled", c.idle.lockEnabled},
             {"lock_timeout_s", c.idle.lockTimeoutS},
             {"screen_off_enabled", c.idle.screenOffEnabled},
             {"screen_off_timeout_s", c.idle.screenOffTimeoutS},
             {"suspend_enabled", c.idle.suspendEnabled},
             {"suspend_timeout_s", c.idle.suspendTimeoutS},
             {"inhibit_while_playing", c.idle.inhibitWhilePlaying},
         }},
        {"recorder",
         toml::table{{"dir", c.recorder.dir}, {"fps", c.recorder.fps}, {"audio", c.recorder.audio}}},
        {"notifications",
         toml::table{
             {"history_limit", c.notifications.historyLimit},
             {"popup_timeout_ms", c.notifications.popupTimeoutMs},
             {"dnd", c.notifications.dnd},
             {"rules", rules},
         }},
        {"theme",
         toml::table{
             {"scheme", c.theme.scheme},
             {"mode", c.theme.mode},
             {"auto_mode", c.theme.autoMode},
             {"builtin_templates", c.theme.builtinTemplates},
             {"templates", templates},
             {"community_templates", c.theme.communityTemplates},
             {"community", community},
         }},
        {"wallpaper",
         toml::table{
             {"dir", c.wallpaper.dir},
             {"current", c.wallpaper.current},
         }},
    };
  }

  Config fromTable(const toml::table& t) {
    Config c;
    const auto island = t["island"];
    c.island.hoverExpand = island["hover_expand"].value_or(c.island.hoverExpand);
    c.island.hoverDelayMs = island["hover_delay_ms"].value_or(c.island.hoverDelayMs);
    c.island.timeFormat = island["time_format"].value_or(c.island.timeFormat);
    if (const auto* binds = t["keybinds"].as_table()) {
      for (const auto& [action, value] : *binds) {
        if (const auto keys = value.value<std::string>()) {
          c.keybinds[std::string(action.str())] = *keys;
        }
      }
    }
    c.calendar.firstDayOfWeek = t["calendar"]["first_day_of_week"].value_or(c.calendar.firstDayOfWeek);
    const auto location = t["location"];
    c.location.latitude = location["latitude"].value<double>();
    c.location.longitude = location["longitude"].value<double>();
    c.location.customSchedule = location["custom_schedule"].value_or(c.location.customSchedule);
    c.location.sunrise = location["sunrise"].value_or(c.location.sunrise);
    c.location.sunset = location["sunset"].value_or(c.location.sunset);
    const auto lock = t["lockscreen"];
    c.lockscreen.wallpaper = lock["wallpaper"].value_or(c.lockscreen.wallpaper);
    c.lockscreen.dnd = lock["dnd"].value_or(c.lockscreen.dnd);
    c.lockscreen.fingerprint = lock["fingerprint"].value_or(c.lockscreen.fingerprint);
    c.lockscreen.monitors = lock["monitors"].value_or(c.lockscreen.monitors);
    const auto idle = t["idle"];
    c.idle.lockEnabled = idle["lock_enabled"].value_or(c.idle.lockEnabled);
    c.idle.lockTimeoutS = idle["lock_timeout_s"].value_or(c.idle.lockTimeoutS);
    c.idle.screenOffEnabled = idle["screen_off_enabled"].value_or(c.idle.screenOffEnabled);
    c.idle.screenOffTimeoutS = idle["screen_off_timeout_s"].value_or(c.idle.screenOffTimeoutS);
    c.idle.suspendEnabled = idle["suspend_enabled"].value_or(c.idle.suspendEnabled);
    c.idle.suspendTimeoutS = idle["suspend_timeout_s"].value_or(c.idle.suspendTimeoutS);
    c.idle.inhibitWhilePlaying = idle["inhibit_while_playing"].value_or(c.idle.inhibitWhilePlaying);
    c.recorder.dir = t["recorder"]["dir"].value_or(c.recorder.dir);
    c.recorder.fps = t["recorder"]["fps"].value_or(c.recorder.fps);
    c.recorder.audio = t["recorder"]["audio"].value_or(c.recorder.audio);
    const auto notifications = t["notifications"];
    c.notifications.historyLimit = notifications["history_limit"].value_or(c.notifications.historyLimit);
    c.notifications.popupTimeoutMs = notifications["popup_timeout_ms"].value_or(c.notifications.popupTimeoutMs);
    c.notifications.dnd = notifications["dnd"].value_or(c.notifications.dnd);
    if (const auto* rules = notifications["rules"].as_array()) {
      for (const auto& node : *rules) {
        if (const auto* rule = node.as_table(); rule != nullptr && (*rule)["app"].is_string()) {
          c.notifications.rules.push_back({(*rule)["app"].value_or(std::string()),
                                           (*rule)["action"].value_or(std::string("mute"))});
        }
      }
    }
    const auto theme = t["theme"];
    c.theme.scheme = theme["scheme"].value_or(c.theme.scheme);
    c.theme.mode = theme["mode"].value_or(c.theme.mode);
    c.theme.autoMode = theme["auto_mode"].value_or(c.theme.autoMode);
    c.theme.builtinTemplates = theme["builtin_templates"].value_or(c.theme.builtinTemplates);
    if (const auto* arr = theme["templates"].as_array()) {
      for (const auto& v : *arr) {
        if (const auto id = v.value<std::string>()) {
          c.theme.templates.push_back(*id);
        }
      }
    }
    c.theme.communityTemplates = theme["community_templates"].value_or(c.theme.communityTemplates);
    if (const auto* arr = theme["community"].as_array()) {
      for (const auto& v : *arr) {
        if (const auto id = v.value<std::string>()) {
          c.theme.community.push_back(*id);
        }
      }
    }
    const auto wallpaper = t["wallpaper"];
    c.wallpaper.dir = wallpaper["dir"].value_or(c.wallpaper.dir);
    c.wallpaper.current = wallpaper["current"].value_or(c.wallpaper.current);
    return c;
  }

  // Writes a copy of `value` at a dotted path, creating the tables on the way.
  void assignAt(toml::table& root, std::string_view path, const toml::node& value) {
    toml::table* table = &root;
    auto parts = std::views::split(path, '.') | std::ranges::to<std::vector<std::string>>();
    for (std::size_t i = 0; i + 1 < parts.size(); ++i) {
      auto* next = (*table)[parts[i]].as_table();
      if (next == nullptr) {
        table->insert_or_assign(parts[i], toml::table{});
        next = (*table)[parts[i]].as_table();
      }
      table = next;
    }
    value.visit([&](const auto& v) { table->insert_or_assign(parts.back(), v); });
  }

} // namespace

toml::table ConfigService::asTable() const { return toTable(m_config); }

const toml::table& ConfigService::defaults() {
  static const toml::table table = toTable(Config{});
  return table;
}

void ConfigService::set(std::string_view path, const toml::node& value) {
  toml::table table = asTable();
  assignAt(table, path, value);
  m_config = fromTable(table);
  save();
}

void ConfigService::reset(std::string_view path) {
  if (const auto* def = defaults().at_path(path).node(); def != nullptr) {
    set(path, *def);
    return;
  }
  // No default (an unset optional such as location.latitude): remove the key.
  toml::table table = asTable();
  const auto dot = path.rfind('.');
  auto* parent = dot == std::string_view::npos ? &table : table.at_path(path.substr(0, dot)).as_table();
  if (parent != nullptr) {
    parent->erase(path.substr(dot == std::string_view::npos ? 0 : dot + 1));
    m_config = fromTable(table);
    save();
  }
}

bool ConfigService::overridden(std::string_view path) const {
  const toml::table table = asTable();
  return table.at_path(path) != defaults().at_path(path);
}

std::string ConfigService::expandHome(const std::string& path) {
  const char* home = std::getenv("HOME");
  if (home != nullptr && path.starts_with("~")) {
    return std::string(home) + path.substr(1);
  }
  return path;
}

std::string ConfigService::filePath() { return FileUtils::configDir() + "/" + kFileName; }

ConfigService::ConfigService() {
  const std::filesystem::path dir = FileUtils::configDir();
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  if (!std::filesystem::exists(filePath())) {
    save();
  }
  load();
  // Watch the directory, not the file: editors replace the file (rename) instead of writing in place.
  m_inotify.watch(dir, IN_CLOSE_WRITE | IN_MOVED_TO);
}

void ConfigService::load() {
  try {
    m_config = fromTable(toml::parse_file(filePath()));
  } catch (const toml::parse_error& e) {
    // Keep the last good config so a typo while editing doesn't reset everything.
    kLog.warn("{}: {}", filePath(), e.description());
  }
}

void ConfigService::save() {
  std::ofstream out(filePath(), std::ios::trunc);
  out << "# Dynamic-Larpland settings. Edited by the settings window; changes here apply live.\n\n"
      << toTable(m_config) << "\n";
}

void ConfigService::doAddPollFds(std::vector<pollfd>& fds) {
  if (m_inotify.fd() >= 0) {
    fds.push_back({.fd = m_inotify.fd(), .events = POLLIN, .revents = 0});
  }
}

void ConfigService::dispatch(const std::vector<pollfd>& fds, std::size_t startIdx) {
  if (startIdx >= fds.size() || (fds[startIdx].revents & POLLIN) == 0) {
    return;
  }
  bool changed = false;
  m_inotify.drain([&changed](const inotify_event* event) {
    changed = changed || (event->len > 0 && std::string_view(event->name) == kFileName);
  });
  if (!changed) {
    return;
  }
  load();
  if (m_onChange) {
    m_onChange();
  }
}
