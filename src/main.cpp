// Dynamic-Larpland: a Dynamic Island for Wayland compositors.
//   dynamic-larpland              run the island
//   dynamic-larpland theme <image>  palette JSON / apply app templates (see --help)
//   dynamic-larpland msg <action> send an action to the running island (toggle, expand, collapse,
//                                 player, calendar, wallpapers, power, launcher, record, lock, quit,
//                                 wallpaper <path>)
#include "app/application.h"
#include "app/library.h"
#include "app/main_loop.h"
#include "auth/pam_authenticator.h"
#include "core/log.h"
#include "theme/theme_cli.h"

#include <csignal>
#include <cstdio>
#include <exception>
#include <string>
#include <string_view>

int main(int argc, char** argv) {
  configureLibrary();

  // PAM runs in a re-exec'd helper (see PamAuthenticator) so it never shares state with the UI threads.
  if (argc >= 2 && std::string_view(argv[1]) == "pam-helper") {
    return PamAuthenticator::runHelperMode(argc, argv);
  }
  if (argc >= 2 && std::string_view(argv[1]) == "theme") {
    return runThemeCli(argc, argv);
  }
  if (argc >= 2 && std::string_view(argv[1]) == "msg") {
    if (argc < 3) {
      std::fprintf(stderr, "usage: dynamic-larpland msg <action> [argument]\n");
      return 2;
    }
    std::string action = argv[2];
    for (int i = 3; i < argc; ++i) {
      action += std::string(" ") + argv[i];
    }
    return ipcSend(action);
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
