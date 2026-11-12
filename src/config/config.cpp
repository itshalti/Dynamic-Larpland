#include "config/config.h"

#include "core/log.h"
#include "util/file_utils.h"

#include <filesystem>
#include <fstream>
#include <toml++/toml.hpp>

namespace {

  constexpr Logger kLog("config");
  constexpr const char* kFileName = "config.toml";

  toml::table toTable(const Config& c) {
    return toml::table{
        {"island",
         toml::table{
             {"hover_expand", c.island.hoverExpand},
             {"hover_delay_ms", c.island.hoverDelayMs},
             {"time_format", c.island.timeFormat},
         }},
    };
  }

  Config fromTable(const toml::table& t) {
    Config c;
    const auto island = t["island"];
    c.island.hoverExpand = island["hover_expand"].value_or(c.island.hoverExpand);
    c.island.hoverDelayMs = island["hover_delay_ms"].value_or(c.island.hoverDelayMs);
    c.island.timeFormat = island["time_format"].value_or(c.island.timeFormat);
    return c;
  }

} // namespace

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
