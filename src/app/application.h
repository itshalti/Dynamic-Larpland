#pragma once

#include "app/deferred_call_poll_source.h"
#include "app/timer_poll_source.h"
#include "config/config.h"
#include "ipc/ipc.h"
#include "island/island.h"
#include "render/gl_shared_context.h"
#include "render/render_context.h"
#include "wayland/wayland_connection.h"

#include <string>

// Owns the Wayland connection, the GL context and every service/UI part, and runs the main loop.
class Application {
public:
  Application();
  ~Application();

  void run();

private:
  std::string handleAction(const std::string& action);

  WaylandConnection m_wayland;
  GlSharedContext m_glShared;
  RenderContext m_renderContext;
  ConfigService m_config;
  TimerPollSource m_timers;
  DeferredCallPollSource m_deferred;
  IpcServer m_ipc;
  Islands m_islands;
};
