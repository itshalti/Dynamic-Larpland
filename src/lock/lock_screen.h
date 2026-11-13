#pragma once

#include "core/timer_manager.h"
#include "render/animation/animation_manager.h"
#include "render/scene/node.h"
#include "wayland/session_lock.h"

#include <memory>
#include <string>
#include <vector>

class AsyncTextureCache;
class Box;
class FingerprintAuthenticator;
class SystemBus;
class ConfigService;
class Label;
class RenderContext;
class WallpaperService;
class WaylandConnection;
struct KeyboardEvent;
struct PointerEvent;
struct ext_session_lock_v1;

// Lock screen (ext-session-lock-v1): on every output the current wallpaper (dimmed) with a big clock and the
// date; typing reveals a pill field at the bottom showing one * per character. Enter checks the password
// through PAM (the "login" stack) off the UI thread; a wrong one shakes the field.
class LockScreen {
public:
  void initialize(WaylandConnection& wayland, RenderContext& renderContext, AsyncTextureCache& textures,
                  WallpaperService& wallpapers, ConfigService& config, SystemBus* systemBus);
  LockScreen();
  ~LockScreen();

  bool active() const noexcept { return m_lock != nullptr; }
  // focusedConnector: output that shows the clock and the field when lockscreen.monitors = "focused".
  void lock(const std::string& focusedConnector = {});

  bool onPointerEvent(const PointerEvent& event);
  bool onKeyboardEvent(const KeyboardEvent& event);

  // ext_session_lock_v1 listener
  static void handleLocked(void* data, ext_session_lock_v1* lock);
  static void handleFinished(void* data, ext_session_lock_v1* lock);

private:
  struct Instance {
    std::unique_ptr<LockSurface> surface;
    // false: a plain black screen (lockscreen.monitors = "focused" on the other outputs).
    bool content = true;
    AnimationManager animations;
    std::unique_ptr<Node> root;
    Label* clock = nullptr;
    Label* date = nullptr;
    Node* fieldGroup = nullptr;
    Box* field = nullptr;
    Label* stars = nullptr;
    Label* error = nullptr;
    Label* hint = nullptr;
  };

  void buildScene(Instance& inst);
  void refresh(Instance& inst);
  void refreshAll();
  void submit();
  void unlock();
  void destroy();
  void clearPassword();
  std::string wallpaperPath() const;

  WaylandConnection* m_wayland = nullptr;
  RenderContext* m_renderContext = nullptr;
  AsyncTextureCache* m_textures = nullptr;
  WallpaperService* m_wallpapers = nullptr;
  ConfigService* m_config = nullptr;
  SystemBus* m_systemBus = nullptr;
  std::unique_ptr<FingerprintAuthenticator> m_fingerprint;
  // Fingerprint status shown under the date ("Touch the fingerprint sensor").
  std::string m_hint;

  ext_session_lock_v1* m_lock = nullptr;
  std::vector<std::unique_ptr<Instance>> m_instances;
  std::string m_password;
  std::string m_error;
  bool m_checking = false;
  std::uint64_t m_generation = 0;
  Timer m_clockTimer;
};
