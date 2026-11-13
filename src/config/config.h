#pragma once

#include "core/inotify/inotify.h"
#include "config/toml.h"
#include "core/poll_source.h"

#include <functional>
#include <string>
#include <map>
#include <optional>
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
  // Switch mode at sunset/sunrise (location.*).
  bool autoMode = false;
  bool builtinTemplates = true;
  // Built-in app templates recolored on every wallpaper pick (ids from assets/templates/builtin.toml).
  // Empty by default so nothing outside the island is touched until the user opts in.
  std::vector<std::string> templates;
  // Community templates (Noctalia's catalog, downloaded on demand).
  bool communityTemplates = true;
  std::vector<std::string> community;
};

struct WallpaperConfig {
  // Folder shown in the wallpaper picker ("~" = home).
  std::string dir = "~/Pictures";
  // Absolute path of the current wallpaper; "" = the island draws none.
  std::string current;
};

struct CalendarConfig {
  // 0 = Sunday, 1 = Monday.
  int firstDayOfWeek = 0;
};

struct NotificationRule {
  // App name as the app sends it (matched case-insensitively).
  std::string app;
  // "mute": kept in the history, no popup. "block": dropped entirely.
  std::string action = "mute";
};

struct NotificationsConfig {
  int historyLimit = 100;
  int popupTimeoutMs = 4000;
  // Do not disturb: no popups (critical notifications still show); the history keeps everything.
  bool dnd = false;
  std::vector<NotificationRule> rules;
};

struct RecorderConfig {
  // Folder for recordings ("~" = home).
  std::string dir = "~/Videos";
  int fps = 60;
  // gpu-screen-recorder -a value ("" = no audio).
  std::string audio = "default_output";
};

// Idle behaviors (seconds of no input); all off by default, like Noctalia.
struct IdleConfig {
  bool lockEnabled = false;
  int lockTimeoutS = 600;
  bool screenOffEnabled = false;
  int screenOffTimeoutS = 660;
  bool suspendEnabled = false;
  int suspendTimeoutS = 900;
  // Skip idle actions while media plays.
  bool inhibitWhilePlaying = true;
};

struct LockscreenConfig {
  // Image for the lock screen; "" = the desktop wallpaper.
  std::string wallpaper;
  // No notification popups while locked.
  bool dnd = true;
  // Also unlock with the fingerprint reader (fprintd).
  bool fingerprint = false;
  // "all" outputs show the clock and the field, or only the "focused" one (the others stay black).
  std::string monitors = "all";
};

// Where the user is, for the day/night theme switch (system/day_night_schedule).
struct LocationConfig {
  std::optional<double> latitude;
  std::optional<double> longitude;
  // Use the fixed sunrise/sunset times instead of the coordinates.
  bool customSchedule = false;
  std::string sunrise = "07:00";
  std::string sunset = "19:00";
};

struct Config {
  IslandConfig island;
  NotificationsConfig notifications;
  CalendarConfig calendar;
  RecorderConfig recorder;
  IdleConfig idle;
  LockscreenConfig lockscreen;
  LocationConfig location;
  // action -> Hyprland key combo ("SUPER + Space"); "" = unbound. See compositors/keybinds.h.
  std::map<std::string, std::string> keybinds = {
      {"launcher", "SUPER + Space"}, {"toggle", ""}, {"player", ""}, {"calendar", ""},
      {"wallpapers", ""},            {"lock", ""},   {"record", ""},
  };
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
  // Settings by TOML path ("island.hover_expand"), for the settings window's generic rows, search and reset.
  toml::table asTable() const;
  static const toml::table& defaults();
  void set(std::string_view path, const toml::node& value);
  // Back to the default value (or removed when the default has none). Saves.
  void reset(std::string_view path);
  bool overridden(std::string_view path) const;

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
