#pragma once

#include "core/inotify/inotify.h"
#include "core/poll_source.h"

#include <functional>
#include <string>
#include <vector>

// User settings in ~/.config/dynamic-larpland/config.toml. The defaults are the member initializers
// below; the file is created with them on first run and reloaded whenever it changes on disk.
struct IslandConfig {
  bool hoverExpand = true;
  int hoverDelayMs = 150;
  // strftime format of the collapsed clock.
  std::string timeFormat = "%H:%M";
};

struct ThemeConfig {
  // Palette scheme for wallpaper colors: m3-tonal-spot, m3-content, m3-fruit-salad, m3-rainbow, m3-monochrome,
  // vibrant, faithful, soft, dysfunctional, muted.
  std::string scheme = "m3-tonal-spot";
  // dark | light: which palette variant the island and the app templates use.
  std::string mode = "dark";
  bool builtinTemplates = true;
  // Built-in app templates recolored on every wallpaper pick (ids from assets/templates/builtin.toml).
  // Empty by default so nothing outside the island is touched until the user opts in.
  std::vector<std::string> templates;
};

struct WallpaperConfig {
  // Folder shown in the wallpaper picker ("~" = home).
  std::string dir = "~/Pictures";
  // Absolute path of the current wallpaper; "" = the island draws none.
  std::string current;
};

struct Config {
  IslandConfig island;
  ThemeConfig theme;
  WallpaperConfig wallpaper;
};

class ConfigService final : public PollSource {
public:
  using ChangeCallback = std::function<void()>;

  ConfigService();

  const Config& config() const noexcept { return m_config; }
  void setChangeCallback(ChangeCallback callback) { m_onChange = std::move(callback); }
  // Writes the current values back to the file (settings window edits).
  void save();
  Config& mutableConfig() noexcept { return m_config; }
  // "~/x" -> "$HOME/x".
  static std::string expandHome(const std::string& path);

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
