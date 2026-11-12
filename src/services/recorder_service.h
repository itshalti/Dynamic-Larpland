#pragma once

#include "core/timer_manager.h"

#include <functional>
#include <string>
#include <sys/types.h>

class ConfigService;

// Screen recording through gpu-screen-recorder: toggle() records the given monitor (desktop audio) into
// recorder.dir; toggle() again sends SIGINT, which makes it finish and save the file.
class RecorderService {
public:
  using ChangeCallback = std::function<void()>;

  explicit RecorderService(ConfigService& config);
  ~RecorderService();

  bool recording() const noexcept { return m_pid > 0; }
  const std::string& lastFile() const noexcept { return m_file; }
  // monitor: connector name ("DP-1"), or "" for gpu-screen-recorder's default ("screen").
  void toggle(const std::string& monitor);
  void setChangeCallback(ChangeCallback callback) { m_onChange = std::move(callback); }

private:
  void poll();

  ConfigService& m_config;
  pid_t m_pid = -1;
  std::string m_file;
  Timer m_watch;
  ChangeCallback m_onChange;
};
