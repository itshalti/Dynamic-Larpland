// ConfigService path operations used by the settings window (set / overridden / reset).
// Run with XDG_CONFIG_HOME pointing at an empty temporary directory (meson test does that).
#include "app/library.h"
#include "config/config.h"

#include <cassert>
#include <cstdio>

int main() {
  configureLibrary();
  ConfigService config;

  assert(!config.overridden("island.hover_delay_ms"));
  config.set("island.hover_delay_ms", toml::value<std::int64_t>(400));
  assert(config.config().island.hoverDelayMs == 400);
  assert(config.overridden("island.hover_delay_ms"));
  assert(!config.overridden("island.hover_expand"));

  // Nested tables and strings, and reading the value back through the table.
  config.set("theme.mode", toml::value<std::string>("light"));
  assert(config.config().theme.mode == "light");
  assert(config.asTable().at_path("theme.mode").value_or(std::string()) == "light");

  config.reset("island.hover_delay_ms");
  assert(config.config().island.hoverDelayMs == 150);
  assert(!config.overridden("island.hover_delay_ms"));
  // reset of one path leaves the others alone
  assert(config.config().theme.mode == "light");

  // Saved to disk: a fresh service reads the same values.
  ConfigService reloaded;
  assert(reloaded.config().theme.mode == "light");
  assert(reloaded.config().island.hoverDelayMs == 150);

  std::puts("config paths: ok");
  return 0;
}
