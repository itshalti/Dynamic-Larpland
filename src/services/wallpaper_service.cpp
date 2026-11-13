#include "services/wallpaper_service.h"

#include "config/config.h"
#include "core/deferred_call.h"
#include "core/log.h"
#include "system/process.h"
#include "util/file_utils.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace {

  constexpr Logger kLog("wallpaper");
  constexpr std::array<std::string_view, 6> kExtensions = {".jpg", ".jpeg", ".png", ".webp", ".bmp", ".jxl"};

  bool isImage(const std::filesystem::path& path) {
    std::string ext = path.extension().string();
    std::ranges::transform(ext, ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return std::ranges::find(kExtensions, ext) != kExtensions.end();
  }

  std::optional<Color> parseHex(const nlohmann::json& value) {
    if (!value.is_string()) {
      return std::nullopt;
    }
    const std::string s = value.get<std::string>();
    if (s.size() != 7 || s[0] != '#') {
      return std::nullopt;
    }
    return rgbHex(static_cast<std::uint32_t>(std::stoul(s.substr(1), nullptr, 16)));
  }

} // namespace

WallpaperService::WallpaperService(ConfigService& config) : m_config(config) {
  rescan();
  loadPalette();
}

std::string WallpaperService::palettePath() { return FileUtils::stateDir() + "/palette.json"; }

const std::string& WallpaperService::current() const { return m_config.config().wallpaper.current; }

std::string WallpaperService::dirDisplay() const {
  // Show "~/..." instead of the full home path.
  const std::string dir = ConfigService::expandHome(m_config.config().wallpaper.dir);
  const char* home = std::getenv("HOME");
  if (home != nullptr && dir.starts_with(std::string(home) + "/")) {
    return "~" + dir.substr(std::string(home).size());
  }
  return dir;
}

void WallpaperService::rescan() {
  m_scannedDir = m_config.config().wallpaper.dir;
  m_images.clear();
  std::error_code ec;
  for (const auto& entry : std::filesystem::directory_iterator(ConfigService::expandHome(m_scannedDir), ec)) {
    if (entry.is_regular_file(ec) && isImage(entry.path())) {
      m_images.push_back(entry.path().string());
    }
  }
  std::ranges::sort(m_images);
}

void WallpaperService::onConfigChanged() {
  if (m_config.config().wallpaper.dir != m_scannedDir) {
    rescan();
  }
  loadPalette();
  notify();
}

void WallpaperService::set(const std::string& path) {
  auto& cfg = m_config.mutableConfig();
  cfg.wallpaper.current = path;
  m_config.save();
  notify();

  std::error_code ec;
  std::filesystem::create_directories(FileUtils::stateDir(), ec);
  std::vector<std::string> args = {
      std::filesystem::read_symlink("/proc/self/exe", ec).string(), "theme", path, "--scheme", cfg.theme.scheme, "--mode", cfg.theme.mode, "-o", palettePath(),
  };
  if (cfg.theme.communityTemplates && !cfg.theme.community.empty()) {
    std::string ids;
    for (const auto& id : cfg.theme.community) {
      ids += (ids.empty() ? "" : ",") + id;
    }
    args.insert(args.end(), {"--community", ids});
  }
  if (cfg.theme.builtinTemplates && !cfg.theme.templates.empty()) {
    std::string ids;
    for (const auto& id : cfg.theme.templates) {
      ids += (ids.empty() ? "" : ",") + id;
    }
    args.insert(args.end(), {"--apply", ids});
  }
  const bool started = process::runAsync(
      args,
      {.stdErr = [](std::string_view chunk) { kLog.debug("theme: {}", chunk); },
       .onExit =
           [this](process::RunResult result) {
             // onExit runs on a worker thread; reload on the UI thread.
             DeferredCall::callLater([this, code = result.exitCode] {
               if (code != 0) {
                 kLog.warn("theme generation exited with {}", code);
               }
               loadPalette();
               notify();
             });
           }}
  );
  if (!started) {
    kLog.warn("could not start theme generation for {}", path);
  }
}

void WallpaperService::loadPalette() {
  std::ifstream in(palettePath());
  if (!in) {
    return;
  }
  try {
    const auto json = nlohmann::json::parse(in);
    const auto& variant = json.contains(m_config.config().theme.mode) ? json[m_config.config().theme.mode] : json;
    m_accent = parseHex(variant.value("primary", nlohmann::json()));
    m_onAccent = parseHex(variant.value("on_primary", nlohmann::json()));
  } catch (const nlohmann::json::exception& e) {
    kLog.warn("{}: {}", palettePath(), e.what());
  }
}

void WallpaperService::notify() {
  if (m_onChange) {
    m_onChange();
  }
}
