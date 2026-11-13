#include "compositors/hyprland.h"

#include "core/log.h"

#include <cstdlib>
#include <cstring>
#include <nlohmann/json.hpp>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace {

  constexpr Logger kLog("hyprland");

  std::string socketPath(const char* name) {
    const char* runtime = std::getenv("XDG_RUNTIME_DIR");
    const char* signature = std::getenv("HYPRLAND_INSTANCE_SIGNATURE");
    if (runtime == nullptr || signature == nullptr) {
      return {};
    }
    return std::string(runtime) + "/hypr/" + signature + "/" + name;
  }

  int connectTo(const std::string& path, bool nonBlocking) {
    sockaddr_un addr{};
    if (path.empty() || path.size() >= sizeof(addr.sun_path)) {
      return -1;
    }
    addr.sun_family = AF_UNIX;
    std::memcpy(addr.sun_path, path.c_str(), path.size() + 1);
    const int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | (nonBlocking ? SOCK_NONBLOCK : 0), 0);
    if (fd < 0) {
      return -1;
    }
    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0 && errno != EINPROGRESS) {
      ::close(fd);
      return -1;
    }
    return fd;
  }

} // namespace

bool HyprlandService::available() { return std::getenv("HYPRLAND_INSTANCE_SIGNATURE") != nullptr; }

HyprlandService::HyprlandService() {
  if (!available()) {
    return;
  }
  queryFocused();
  m_events = connectTo(socketPath(".socket2.sock"), true);
  if (m_events < 0) {
    kLog.warn("cannot connect to the Hyprland event socket");
  }
}

HyprlandService::~HyprlandService() {
  if (m_events >= 0) {
    ::close(m_events);
  }
}

std::string HyprlandService::request(const std::string& command) {
  const int fd = connectTo(socketPath(".socket.sock"), false);
  if (fd < 0) {
    return {};
  }
  (void)::write(fd, command.data(), command.size());
  std::string reply;
  char buf[4096];
  pollfd pfd{.fd = fd, .events = POLLIN, .revents = 0};
  while (::poll(&pfd, 1, 500) > 0) {
    const ssize_t n = ::read(fd, buf, sizeof(buf));
    if (n <= 0) {
      break;
    }
    reply.append(buf, static_cast<std::size_t>(n));
  }
  ::close(fd);
  return reply;
}

bool HyprlandService::setDpms(bool on) {
  // Lua configs (0.55+) take hl.dsp.* dispatchers; classic configs the old dispatcher syntax.
  const std::string lua = std::string("dispatch hl.dsp.dpms({ action = \"") + (on ? "enable" : "disable") + "\" })";
  if (request(lua).starts_with("ok")) {
    return true;
  }
  return request(std::string("dispatch dpms ") + (on ? "on" : "off")).starts_with("ok");
}

void HyprlandService::queryFocused() {
  // "j/monitors" -> JSON array with a "focused" flag.
  const std::string reply = request("j/monitors");
  try {
    for (const auto& monitor : nlohmann::json::parse(reply)) {
      if (monitor.value("focused", false)) {
        m_focused = monitor.value("name", "");
      }
    }
  } catch (const nlohmann::json::exception& e) {
    kLog.warn("unexpected monitors reply: {}", e.what());
  }
}

void HyprlandService::doAddPollFds(std::vector<pollfd>& fds) {
  if (m_events >= 0) {
    fds.push_back({.fd = m_events, .events = POLLIN, .revents = 0});
  }
}

void HyprlandService::dispatch(const std::vector<pollfd>& fds, std::size_t startIdx) {
  if (m_events < 0 || startIdx >= fds.size() || fds[startIdx].revents == 0) {
    return;
  }
  char buf[4096];
  const ssize_t n = ::read(m_events, buf, sizeof(buf));
  if (n <= 0) {
    if (n == 0 || (errno != EAGAIN && errno != EINTR)) {
      ::close(m_events);
      m_events = -1;
    }
    return;
  }
  m_buffer.append(buf, static_cast<std::size_t>(n));
  std::size_t nl = 0;
  while ((nl = m_buffer.find('\n')) != std::string::npos) {
    handleLine(m_buffer.substr(0, nl));
    m_buffer.erase(0, nl + 1);
  }
}

void HyprlandService::handleLine(const std::string& line) {
  // focusedmon>>MONITOR,WORKSPACE
  constexpr std::string_view kEvent = "focusedmon>>";
  if (line.starts_with(kEvent)) {
    const auto rest = line.substr(kEvent.size());
    m_focused = rest.substr(0, rest.find(','));
  }
}
