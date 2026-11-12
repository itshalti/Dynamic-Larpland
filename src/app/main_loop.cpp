#include "app/main_loop.h"

#include "core/deferred_call.h"
#include "core/log.h"
#include "core/poll_source.h"
#include "wayland/surface.h"
#include "wayland/wayland_connection.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <format>
#include <limits>
#include <poll.h>
#include <stdexcept>
#include <wayland-client-core.h>

namespace {

  constexpr Logger kLog("main");
  std::atomic<bool> g_quit{false};

  // A C++ exception escaping a Wayland listener would unwind through libffi and terminate;
  // contain it so one bad handler can't kill the island.
  int dispatchPendingGuarded(wl_display* display) {
    try {
      return wl_display_dispatch_pending(display);
    } catch (const std::exception& e) {
      kLog.error("exception in Wayland event handler ({}); continuing", e.what());
      return 0;
    }
  }

  bool isDisconnect(int err) { return err == EPIPE || err == ECONNRESET; }

} // namespace

MainLoop::MainLoop(WaylandConnection& wayland, PollSourcesProvider sourcesProvider)
    : m_wayland(wayland), m_sourcesProvider(std::move(sourcesProvider)) {}

void MainLoop::requestQuit() noexcept { g_quit = true; }

void MainLoop::dispatchPending() {
  if (dispatchPendingGuarded(m_wayland.display()) < 0) {
    const int err = errno;
    if (isDisconnect(err)) {
      kLog.info("Wayland display closed; shutting down");
      g_quit = true;
      return;
    }
    throw std::runtime_error(std::format("Wayland dispatch failed: {}", m_wayland.describeDisplayError(err)));
  }
}

void MainLoop::run() {
  wl_display* display = m_wayland.display();
  while (!g_quit) {
    for (auto& fn : DeferredCall::takePending()) {
      if (fn) {
        fn();
      }
    }

    while (wl_display_prepare_read(display) != 0) {
      dispatchPending();
      if (g_quit) {
        return;
      }
    }

    // EAGAIN = kernel buffer full: wait for POLLOUT and retry instead of failing.
    short waylandEvents = POLLIN;
    int flushRet = 0;
    do {
      flushRet = wl_display_flush(display);
    } while (flushRet < 0 && errno == EINTR);
    if (flushRet < 0) {
      const int err = errno;
      if (err != EAGAIN) {
        wl_display_cancel_read(display);
        if (isDisconnect(err)) {
          return;
        }
        throw std::runtime_error(std::format("Wayland flush failed: {}", m_wayland.describeDisplayError(err)));
      }
      waylandEvents |= POLLOUT;
    }

    const std::vector<PollSource*> sources = m_sourcesProvider ? m_sourcesProvider() : std::vector<PollSource*>{};
    std::vector<pollfd> fds;
    fds.push_back({.fd = wl_display_get_fd(display), .events = waylandEvents, .revents = 0});
    std::vector<std::size_t> starts;
    std::vector<std::size_t> counts;

    int timeout = -1;
    const auto now = std::chrono::steady_clock::now();
    for (auto* source : sources) {
      const std::size_t start = source->addPollFds(fds);
      starts.push_back(start);
      counts.push_back(fds.size() - start);
      const int t = source->pollTimeoutMs();
      if (t < 0) {
        m_sourceDeadlines.erase(source);
        continue;
      }
      // A source's timeout is an absolute deadline from when it was first advertised.
      auto [it, inserted] = m_sourceDeadlines.try_emplace(source, now + std::chrono::milliseconds(t));
      if (!inserted) {
        it->second = std::min(it->second, now + std::chrono::milliseconds(t));
      }
      const auto left = std::chrono::ceil<std::chrono::milliseconds>(it->second - now).count();
      const int leftMs = left < 0 ? 0 : static_cast<int>(std::min<long long>(left, std::numeric_limits<int>::max()));
      if (timeout < 0 || leftMs < timeout) {
        timeout = leftMs;
      }
    }
    if (Surface::hasPendingFrameWork() || Surface::hasPendingRenders()) {
      timeout = 0;
    }
    if ((waylandEvents & POLLOUT) != 0 && timeout >= 0 && timeout < 16) {
      timeout = 16;
    }

    if (::poll(fds.data(), fds.size(), timeout) < 0) {
      wl_display_cancel_read(display);
      if (errno == EINTR) {
        continue;
      }
      throw std::runtime_error("poll failed");
    }

    if ((fds[0].revents & (POLLIN | POLLERR | POLLHUP)) != 0) {
      if (wl_display_read_events(display) < 0) {
        if (isDisconnect(errno)) {
          return;
        }
        throw std::runtime_error("failed to read Wayland events");
      }
    } else {
      wl_display_cancel_read(display);
    }
    dispatchPending();
    if ((fds[0].revents & (POLLHUP | POLLERR)) != 0) {
      kLog.info("Wayland socket hung up; shutting down");
      return;
    }

    const auto after = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < sources.size(); ++i) {
      bool ready = false;
      for (std::size_t f = starts[i]; f < starts[i] + counts[i] && f < fds.size(); ++f) {
        ready = ready || fds[f].revents != 0;
      }
      const auto deadline = m_sourceDeadlines.find(sources[i]);
      const bool timedOut = deadline != m_sourceDeadlines.end() && after >= deadline->second;
      if (!ready && !timedOut) {
        continue;
      }
      m_sourceDeadlines.erase(sources[i]);
      sources[i]->dispatch(fds, starts[i]);
    }

    Surface::drainPendingFrameWork();
    Surface::drainPendingRenders();
  }
  dispatchPendingGuarded(display);
  wl_display_flush(display);
}
