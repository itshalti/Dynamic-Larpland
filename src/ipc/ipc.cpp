#include "ipc/ipc.h"

#include "core/log.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace {

  constexpr Logger kLog("ipc");
  constexpr std::size_t kMaxLine = 1024;

  bool fillAddress(const std::string& path, sockaddr_un& addr) {
    if (path.size() >= sizeof(addr.sun_path)) {
      return false;
    }
    addr = {};
    addr.sun_family = AF_UNIX;
    std::memcpy(addr.sun_path, path.c_str(), path.size() + 1);
    return true;
  }

  // Reads one line (without the newline) with a short timeout; clients are local and tiny.
  std::string readLine(int fd) {
    std::string line;
    char c = 0;
    pollfd pfd{.fd = fd, .events = POLLIN, .revents = 0};
    while (line.size() < kMaxLine && ::poll(&pfd, 1, 500) > 0 && ::read(fd, &c, 1) == 1 && c != '\n') {
      line.push_back(c);
    }
    return line;
  }

} // namespace

std::string IpcServer::socketPath() {
  const char* runtime = std::getenv("XDG_RUNTIME_DIR");
  const char* display = std::getenv("WAYLAND_DISPLAY");
  return std::string(runtime != nullptr ? runtime : "/tmp") + "/dynamic-larpland-"
         + (display != nullptr ? display : "wayland-0") + ".sock";
}

IpcServer::~IpcServer() {
  if (m_fd >= 0) {
    ::close(m_fd);
    ::unlink(m_path.c_str());
  }
}

bool IpcServer::start(Handler handler) {
  m_handler = std::move(handler);
  m_path = socketPath();
  sockaddr_un addr{};
  if (!fillAddress(m_path, addr)) {
    kLog.error("socket path too long: {}", m_path);
    return false;
  }
  m_fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
  if (m_fd < 0) {
    return false;
  }
  // A stale socket from a crashed instance would make bind fail.
  ::unlink(m_path.c_str());
  if (::bind(m_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0 || ::listen(m_fd, 8) < 0) {
    kLog.error("cannot listen on {}: {}", m_path, std::strerror(errno));
    ::close(m_fd);
    m_fd = -1;
    return false;
  }
  kLog.info("IPC listening on {}", m_path);
  return true;
}

void IpcServer::doAddPollFds(std::vector<pollfd>& fds) {
  if (m_fd >= 0) {
    fds.push_back({.fd = m_fd, .events = POLLIN, .revents = 0});
  }
}

void IpcServer::dispatch(const std::vector<pollfd>& fds, std::size_t startIdx) {
  if (m_fd < 0 || startIdx >= fds.size() || (fds[startIdx].revents & POLLIN) == 0) {
    return;
  }
  while (true) {
    const int client = ::accept4(m_fd, nullptr, nullptr, SOCK_CLOEXEC);
    if (client < 0) {
      return;
    }
    const std::string action = readLine(client);
    const std::string reply = (m_handler ? m_handler(action) : std::string("no handler")) + "\n";
    (void)::write(client, reply.data(), reply.size());
    ::close(client);
  }
}

int ipcSend(const std::string& action) {
  const std::string path = IpcServer::socketPath();
  sockaddr_un addr{};
  const int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (fd < 0 || !fillAddress(path, addr) || ::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    std::fprintf(stderr, "dynamic-larpland is not running (no socket at %s)\n", path.c_str());
    if (fd >= 0) {
      ::close(fd);
    }
    return 1;
  }
  const std::string line = action + "\n";
  (void)::write(fd, line.data(), line.size());
  const std::string reply = readLine(fd);
  ::close(fd);
  std::printf("%s\n", reply.c_str());
  return reply == "ok" ? 0 : 1;
}
