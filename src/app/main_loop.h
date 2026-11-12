#pragma once

#include <chrono>
#include <functional>
#include <unordered_map>
#include <vector>

class PollSource;
class WaylandConnection;

// Poll-based event loop: Wayland fd + every PollSource (timers, deferred calls, IPC, D-Bus...),
// then the queued per-surface frame work and renders. Trimmed from Noctalia's src/app/main_loop.cpp.
class MainLoop {
public:
  using PollSourcesProvider = std::function<std::vector<PollSource*>()>;

  MainLoop(WaylandConnection& wayland, PollSourcesProvider sourcesProvider);

  void run();
  static void requestQuit() noexcept;

private:
  void dispatchPending();

  WaylandConnection& m_wayland;
  PollSourcesProvider m_sourcesProvider;
  std::unordered_map<PollSource*, std::chrono::steady_clock::time_point> m_sourceDeadlines;
};
