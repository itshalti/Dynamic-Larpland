#include "services/recorder_service.h"

#include "config/config.h"
#include "core/log.h"

#include <csignal>
#include <ctime>
#include <filesystem>
#include <format>
#include <spawn.h>
#include <sys/wait.h>
#include <vector>

extern char** environ;

namespace {

  constexpr Logger kLog("recorder");

} // namespace

RecorderService::RecorderService(ConfigService& config) : m_config(config) {}

RecorderService::~RecorderService() {
  // Don't leave a recorder running without a way to stop it; SIGINT still saves the file.
  if (m_pid > 0) {
    ::kill(m_pid, SIGINT);
  }
}

void RecorderService::toggle(const std::string& monitor) {
  if (m_pid > 0) {
    ::kill(m_pid, SIGINT);
    return;
  }
  const auto& cfg = m_config.config().recorder;
  const std::string dir = ConfigService::expandHome(cfg.dir);
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  const std::time_t now = std::time(nullptr);
  std::tm local{};
  localtime_r(&now, &local);
  m_file = std::format("{}/recording_{:04}-{:02}-{:02}_{:02}-{:02}-{:02}.mp4", dir, local.tm_year + 1900,
                       local.tm_mon + 1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);

  std::vector<std::string> args = {"gpu-screen-recorder", "-w", monitor.empty() ? "screen" : monitor,
                                   "-f", std::to_string(cfg.fps), "-o", m_file};
  if (!cfg.audio.empty()) {
    args.insert(args.end(), {"-a", cfg.audio});
  }
  std::vector<char*> argv;
  for (auto& a : args) {
    argv.push_back(a.data());
  }
  argv.push_back(nullptr);
  pid_t pid = -1;
  if (posix_spawnp(&pid, argv[0], nullptr, nullptr, argv.data(), environ) != 0) {
    kLog.warn("could not start gpu-screen-recorder (is it installed?)");
    return;
  }
  m_pid = pid;
  kLog.info("recording {} to {}", monitor.empty() ? "screen" : monitor, m_file);
  m_watch.startRepeating(std::chrono::milliseconds(500), [this] { poll(); });
  if (m_onChange) {
    m_onChange();
  }
}

void RecorderService::poll() {
  int status = 0;
  if (m_pid <= 0 || ::waitpid(m_pid, &status, WNOHANG) != m_pid) {
    return;
  }
  if (!WIFEXITED(status) || (WEXITSTATUS(status) != 0 && WEXITSTATUS(status) != 130)) {
    kLog.warn("gpu-screen-recorder stopped with status {}", status);
  }
  m_pid = -1;
  m_watch.stop();
  if (m_onChange) {
    m_onChange();
  }
}
