#pragma once

#include <string>

struct Config;

// Keybinds: on Hyprland (Lua config) writes ~/.config/dynamic-larpland/binds.lua with one
// `hl.bind(keys, hl.dsp.exec_cmd("<this binary> msg <action>"))` per set keybind and reloads Hyprland when the
// file changed. The user's hyprland.lua loads it with includeLine(). Elsewhere nothing is written; the
// settings page shows the IPC command to bind instead.
namespace keybinds {

  // Actions that can have a keybind, in settings order.
  inline constexpr const char* kActions[] = {"launcher", "toggle", "player", "calendar", "wallpapers", "lock", "record"};

  std::string bindsPath();
  // Line for the user's hyprland.lua.
  std::string includeLine();
  // `dynamic-larpland msg <action>` with the absolute binary path.
  std::string ipcCommand(const std::string& action);
  void writeHyprland(const Config& config);

} // namespace keybinds
