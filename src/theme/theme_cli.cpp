#include "theme/theme_cli.h"

#include "core/files/resource_paths.h"
#include "theme/image_loader.h"
#include "theme/json_output.h"
#include "theme/palette.h"
#include "theme/palette_generator.h"
#include "theme/scheme.h"
#include "theme/template_engine.h"

#include <cstdio>
#include <fstream>
#include <ranges>
#include <string>
#include <string_view>

namespace {

  using namespace noctalia::theme;

  constexpr const char* kUsage = R"(usage: dynamic-larpland theme <image> [options]

Generate a color palette from an image (Material You and custom schemes).

  --scheme <name>     m3-tonal-spot (default), m3-content, m3-fruit-salad, m3-rainbow, m3-monochrome,
                      vibrant, faithful, soft, dysfunctional, muted
  --dark | --light    print only that variant (default: both)
  -o <file>           write the JSON to a file instead of stdout (also works with --apply)
  --apply <ids>       render the given built-in templates (comma separated: gtk3,gtk4,kitty,hyprland,...)
  --mode <dark|light> mode the templates use (default: dark)
)";

} // namespace

int runThemeCli(int argc, char** argv) {
  std::string image;
  Scheme scheme = Scheme::TonalSpot;
  std::string schemeName = "m3-tonal-spot";
  Variant variant = Variant::Both;
  std::string outFile;
  std::string apply;
  std::string mode = "dark";

  for (int i = 2; i < argc; ++i) {
    const std::string_view arg = argv[i];
    auto next = [&]() -> std::string {
      if (i + 1 >= argc) {
        std::fprintf(stderr, "missing value for %.*s\n", static_cast<int>(arg.size()), arg.data());
        return {};
      }
      return argv[++i];
    };
    if (arg == "-h" || arg == "--help") {
      std::fputs(kUsage, stdout);
      return 0;
    }
    if (arg == "--scheme") {
      schemeName = next();
      const auto parsed = schemeFromString(schemeName);
      if (!parsed) {
        std::fprintf(stderr, "unknown scheme: %s\n", schemeName.c_str());
        return 2;
      }
      scheme = *parsed;
    } else if (arg == "--dark") {
      variant = Variant::Dark;
    } else if (arg == "--light") {
      variant = Variant::Light;
    } else if (arg == "-o") {
      outFile = next();
    } else if (arg == "--apply") {
      apply = next();
    } else if (arg == "--mode") {
      mode = next();
    } else if (image.empty() && !arg.starts_with("-")) {
      image = arg;
    } else {
      std::fprintf(stderr, "unexpected argument: %.*s\n%s", static_cast<int>(arg.size()), arg.data(), kUsage);
      return 2;
    }
  }
  if (image.empty()) {
    std::fputs(kUsage, stderr);
    return 2;
  }
  if (mode != "dark" && mode != "light") {
    std::fprintf(stderr, "--mode must be dark or light\n");
    return 2;
  }

  const auto loaded = loadAndResize(image, scheme);
  if (!loaded) {
    std::fprintf(stderr, "%s: %s\n", image.c_str(), loaded.error().c_str());
    return 1;
  }
  const auto palette = generate(loaded->rgb, scheme);
  if (!palette) {
    std::fprintf(stderr, "palette generation failed: %s\n", palette.error().c_str());
    return 1;
  }

  if (!outFile.empty()) {
    std::ofstream(outFile, std::ios::trunc) << toJson(*palette, scheme, variant) << "\n";
  }
  if (!apply.empty()) {
    TemplateEngine::Options options;
    options.defaultMode = mode;
    options.imagePath = image;
    options.schemeType = schemeName.starts_with("m3-") ? schemeName.substr(3) : schemeName;
    for (const auto id : std::views::split(apply, ',')) {
      options.enabledTemplates.emplace(std::string_view(id));
    }
    TemplateEngine engine(TemplateEngine::makeThemeData(*palette), std::move(options));
    const auto catalog = paths::assetPath("templates/builtin.toml");
    if (!engine.processConfigFile(catalog)) {
      std::fprintf(stderr, "applying templates from %s failed\n", catalog.c_str());
      return 1;
    }
    return 0;
  }

  if (outFile.empty()) {
    std::fputs(toJson(*palette, scheme, variant).c_str(), stdout);
    std::fputs("\n", stdout);
  }
  return 0;
}
