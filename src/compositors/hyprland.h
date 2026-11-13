#pragma once

#include "core/poll_source.h"

#include <string>

// Minimal Hyprland IPC: tracks the focused monitor through the event socket (.socket2.sock). Inactive (no
// fds, empty name) on other compositors, where actions then go to every screen.
class HyprlandService final : public PollSource {
public:
  HyprlandService();
  ~HyprlandService() override;

  static bool available();
  // One request on Hyprland's command socket ("j/monitors", "dispatch ..."); "" when it fails.
  static std::string request(const std::string& command);
  // Turns every monitor on or off (idle screen-off). Works with the Lua and the classic config.
  static bool setDpms(bool on);
  // Connector name of the focused monitor ("DP-1"), or "" when unknown.
  const std::string& focusedMonitor() const noexcept { return m_focused; }

  void dispatch(const std::vector<pollfd>& fds, std::size_t startIdx) override;

protected:
  void doAddPollFds(std::vector<pollfd>& fds) override;

private:
  void queryFocused();
  void handleLine(const std::string& line);

  int m_events = -1;
  std::string m_buffer;
  std::string m_focused;
};
