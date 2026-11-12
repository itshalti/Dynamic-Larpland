#pragma once

#include "render/core/color.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

class ConfigService;

// Wallpapers: the images in wallpaper.dir, the current one (drawn by wallpaper/Background) and the palette
// generated from it. Picking a wallpaper runs `dynamic-larpland theme <img> -o palette.json --apply <ids>` as a
// child process (decoding and the app hooks never block the UI); when it exits the palette is reloaded.
class WallpaperService {
public:
  using ChangeCallback = std::function<void()>;

  explicit WallpaperService(ConfigService& config);

  // Image paths in the wallpaper folder, sorted by name.
  const std::vector<std::string>& images() const noexcept { return m_images; }
  const std::string& current() const;
  std::string dirDisplay() const;
  void rescan();
  void set(const std::string& path);

  // Island accent from the current palette (theme.mode variant), when there is one.
  std::optional<Color> accent() const noexcept { return m_accent; }
  std::optional<Color> onAccent() const noexcept { return m_onAccent; }

  // Wallpaper, image list or palette changed.
  void setChangeCallback(ChangeCallback callback) { m_onChange = std::move(callback); }
  void onConfigChanged();

  static std::string palettePath();

private:
  void loadPalette();
  void notify();

  ConfigService& m_config;
  std::vector<std::string> m_images;
  std::string m_scannedDir;
  std::optional<Color> m_accent;
  std::optional<Color> m_onAccent;
  ChangeCallback m_onChange;
};
