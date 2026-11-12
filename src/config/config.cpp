#include "config/config.h"

#include "core/log.h"
#include "util/file_utils.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <toml++/toml.hpp>

namespace {

  constexpr Logger kLog("config");
  constexpr const char* kFileName = "config.toml";

  toml::table toTable(const Config& c) {
    toml::array templates;
    for (const auto& id : c.theme.templates) {
      templates.push_back(id);
    }
    toml::table binds;
    for (const auto& [action, keys] : c.keybinds) {
      binds.insert(action, keys);
    }
    return toml::table{
        {"keybinds", binds},
        {"island",
         toml::table{
             {"hover_expand", c.island.hoverExpand},
             {"hover_delay_ms", c.island.hoverDelayMs},
             {"time_format", c.island.timeFormat},
         }},
        {"calendar", toml::table{{"first_day_of_week", c.calendar.firstDayOfWeek}}},
        {"recorder",
         toml::table{{"dir", c.recorder.dir}, {"fps", c.recorder.fps}, {"audio", c.recorder.audio}}},
        {"notifications",
         toml::table{
             {"history_limit", c.notifications.historyLimit},
             {"popup_timeout_ms", c.notifications.popupTimeoutMs},
         }},
        {"theme",
         toml::table{
             {"scheme", c.theme.scheme},
             {"mode", c.theme.mode},
             {"builtin_templates", c.theme.builtinTemplates},
             {"templates", templates},
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
    c.recorder.dir = t["recorder"]["dir"].value_or(c.recorder.dir);
    c.recorder.fps = t["recorder"]["fps"].value_or(c.recorder.fps);
    c.recorder.audio = t["recorder"]["audio"].value_or(c.recorder.audio);
    const auto notifications = t["notifications"];
    c.notifications.historyLimit = notifications["history_limit"].value_or(c.notifications.historyLimit);
    c.notifications.popupTimeoutMs = notifications["popup_timeout_ms"].value_or(c.notifications.popupTimeoutMs);
    const auto theme = t["theme"];
    c.theme.scheme = theme["scheme"].value_or(c.theme.scheme);
    c.theme.mode = theme["mode"].value_or(c.theme.mode);
    c.theme.builtinTemplates = theme["builtin_templates"].value_or(c.theme.builtinTemplates);
    if (const auto* arr = theme["templates"].as_array()) {
      for (const auto& v : *arr) {
        if (const auto id = v.value<std::string>()) {
          c.theme.templates.push_back(*id);
        }
      }
    }
    const auto wallpaper = t["wallpaper"];
    c.wallpaper.dir = wallpaper["dir"].value_or(c.wallpaper.dir);
    c.wallpaper.current = wallpaper["current"].value_or(c.wallpaper.current);
    return c;
  }

} // namespace

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
