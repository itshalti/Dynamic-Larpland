#pragma once

#include "core/inotify/inotify.h"
#include "core/poll_source.h"

#include <functional>
#include <string>

// User settings in ~/.config/dynamic-larpland/config.toml. The defaults are the member initializers
// below; the file is created with them on first run and reloaded whenever it changes on disk.
struct IslandConfig {
  bool hoverExpand = true;
  int hoverDelayMs = 150;
  // strftime format of the collapsed clock.
  std::string timeFormat = "%H:%M";
};

struct Config {
  IslandConfig island;
};

class ConfigService final : public PollSource {
public:
  using ChangeCallback = std::function<void()>;

  ConfigService();

  const Config& config() const noexcept { return m_config; }
  void setChangeCallback(ChangeCallback callback) { m_onChange = std::move(callback); }
  // Writes the current values back to the file (settings window edits).
  void save();

  static std::string filePath();

  void dispatch(const std::vector<pollfd>& fds, std::size_t startIdx) override;

protected:
  void doAddPollFds(std::vector<pollfd>& fds) override;

private:
  void load();

  Config m_config;
  Inotify m_inotify;
  ChangeCallback m_onChange;
};
