#pragma once

#include "core/poll_source.h"

#include <functional>
#include <string>

// Local IPC: a unix socket per Wayland display ($XDG_RUNTIME_DIR/dynamic-larpland-<display>.sock).
// Each client sends one action line ("toggle\n") and gets one reply line ("ok" or an error).
class IpcServer final : public PollSource {
public:
  // Returns the reply line for an action ("ok", "unknown action: x").
  using Handler = std::function<std::string(const std::string& action)>;

  ~IpcServer() override;

  bool start(Handler handler);
  void dispatch(const std::vector<pollfd>& fds, std::size_t startIdx) override;

  static std::string socketPath();

protected:
  void doAddPollFds(std::vector<pollfd>& fds) override;

private:
  int m_fd = -1;
  std::string m_path;
  Handler m_handler;
};

// `dynamic-larpland msg <action>`: sends the action to the running instance, prints its reply.
int ipcSend(const std::string& action);
