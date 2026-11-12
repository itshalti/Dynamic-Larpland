// Dynamic-Larpland: a Dynamic Island for Wayland compositors.
//   dynamic-larpland              run the island
//   dynamic-larpland msg <action> send an action to the running island (toggle, expand, collapse,
//                                 player, calendar, wallpapers, power, quit)
#include "app/application.h"
#include "app/library.h"
#include "app/main_loop.h"
#include "core/log.h"

#include <csignal>
#include <cstdio>
#include <exception>
#include <string_view>

int main(int argc, char** argv) {
  configureLibrary();

  if (argc >= 2 && std::string_view(argv[1]) == "msg") {
    if (argc < 3) {
      std::fprintf(stderr, "usage: dynamic-larpland msg <action>\n");
      return 2;
    }
    return ipcSend(argv[2]);
  }

  std::signal(SIGINT, [](int) { MainLoop::requestQuit(); });
  std::signal(SIGTERM, [](int) { MainLoop::requestQuit(); });

  try {
    Application app;
    app.run();
  } catch (const std::exception& e) {
    std::fprintf(stderr, "dynamic-larpland: %s\n", e.what());
    return 1;
  }
  return 0;
}
