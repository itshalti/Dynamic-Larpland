#pragma once

// Configures the vendored noctalia-ui library for this app: identity (XDG dirs, log file, env prefix),
// the few UI strings the library shows itself, and the image codecs. Call once, first thing in main().
void configureLibrary();
