#include "lock/lock_screen.h"

#include "auth/fingerprint_authenticator.h"
#include "auth/pam_authenticator.h"
#include "dbus/system_bus.h"
#include "config/config.h"
#include "core/deferred_call.h"
#include "core/log.h"
#include "core/ui_phase.h"
#include "ext-session-lock-v1-client-protocol.h"
#include "island/theme.h"
#include "island/widgets.h"
#include "render/core/async_texture_cache.h"
#include "services/wallpaper_service.h"
#include "ui/controls/box.h"
#include "ui/controls/glyph.h"
#include "ui/controls/image.h"
#include "ui/controls/label.h"
#include "wayland/wayland_connection.h"
#include "wayland/wayland_seat.h"

#include <array>
#include <ctime>
#include <thread>
#include <xkbcommon/xkbcommon-keysyms.h>

namespace {

  constexpr Logger kLog("lock");
  constexpr float kFieldW = 340.0F;
  constexpr float kFieldH = 54.0F;
  constexpr float kFieldMargin = 90.0F;
  constexpr std::size_t kMaxPassword = 512;

  const ext_session_lock_v1_listener kLockListener = {
      .locked = &LockScreen::handleLocked,
      .finished = &LockScreen::handleFinished,
  };

  std::string formatNow(const char* format) {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_r(&now, &local);
    std::array<char, 64> buf{};
    const std::size_t n = std::strftime(buf.data(), buf.size(), format, &local);
    return std::string(buf.data(), n);
  }

  void appendUtf8(std::string& out, std::uint32_t cp) {
    if (cp < 0x80) {
      out += static_cast<char>(cp);
    } else if (cp < 0x800) {
      out += static_cast<char>(0xC0 | (cp >> 6));
      out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
      out += static_cast<char>(0xE0 | (cp >> 12));
      out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
      out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
      out += static_cast<char>(0xF0 | (cp >> 18));
      out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
      out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
      out += static_cast<char>(0x80 | (cp & 0x3F));
    }
  }

  std::size_t utf8Length(const std::string& s) {
    std::size_t n = 0;
    for (const unsigned char c : s) {
      n += (c & 0xC0) != 0x80 ? 1 : 0;
    }
    return n;
  }

} // namespace

void LockScreen::initialize(WaylandConnection& wayland, RenderContext& renderContext, AsyncTextureCache& textures,
                            WallpaperService& wallpapers, ConfigService& config, SystemBus* systemBus) {
  m_systemBus = systemBus;
  m_wayland = &wayland;
  m_renderContext = &renderContext;
  m_textures = &textures;
  m_wallpapers = &wallpapers;
  m_config = &config;
}

LockScreen::LockScreen() = default;
LockScreen::~LockScreen() { clearPassword(); }

std::string LockScreen::wallpaperPath() const {
  const std::string& own = m_config->config().lockscreen.wallpaper;
  return own.empty() ? m_wallpapers->current() : ConfigService::expandHome(own);
}

void LockScreen::lock(const std::string& focusedConnector) {
  if (m_lock != nullptr) {
    return;
  }
  auto* manager = m_wayland->extension<SessionLockManager>().manager();
  if (manager == nullptr) {
    kLog.warn("the compositor does not support ext-session-lock-v1");
    return;
  }
  m_lock = ext_session_lock_manager_v1_lock(manager);
  ext_session_lock_v1_add_listener(m_lock, &kLockListener, this);
  clearPassword();
  m_error.clear();
  m_hint.clear();
  m_checking = false;
  const auto& cfg = m_config->config().lockscreen;

  for (const auto& output : m_wayland->outputs()) {
    if (output.output == nullptr) {
      continue;
    }
    auto inst = std::make_unique<Instance>();
    inst->content = cfg.monitors != "focused" || focusedConnector.empty() || output.connectorName == focusedConnector;
    inst->surface = std::make_unique<LockSurface>(*m_wayland);
    inst->surface->setRenderContext(m_renderContext);
    inst->surface->setAnimationManager(&inst->animations);
    Instance* raw = inst.get();
    inst->surface->setConfigureCallback([raw](std::uint32_t, std::uint32_t) { raw->surface->requestLayout(); });
    inst->surface->setPrepareFrameCallback([this, raw](bool, bool) {
      const auto w = static_cast<float>(raw->surface->width());
      if (raw->root == nullptr || raw->root->width() != w) {
        UiPhaseScope layoutPhase(UiPhase::Layout);
        buildScene(*raw);
      }
    });
    if (inst->surface->initialize(m_lock, output.output)) {
      m_instances.push_back(std::move(inst));
    }
  }
  m_clockTimer.startRepeating(std::chrono::seconds(1), [this] { refreshAll(); });
  if (cfg.fingerprint && m_systemBus != nullptr) {
    m_fingerprint = std::make_unique<FingerprintAuthenticator>(*m_systemBus);
    m_fingerprint->setAuthenticatedCallback([this] { DeferredCall::callLater([this] { unlock(); }); });
    m_fingerprint->setStatusCallback([this](const std::string& message, bool /*isError*/) {
      m_hint = message;
      refreshAll();
    });
    m_fingerprint->start();
  }
  kLog.info("locking ({} outputs)", m_instances.size());
}

void LockScreen::handleLocked(void* /*data*/, ext_session_lock_v1* /*lock*/) { kLog.info("session locked"); }

void LockScreen::handleFinished(void* data, ext_session_lock_v1* /*lock*/) {
  // The compositor refused the lock (another locker is active) or ended it.
  auto* self = static_cast<LockScreen*>(data);
  kLog.warn("session lock finished by the compositor");
  DeferredCall::callLater([self] { self->destroy(); });
}

void LockScreen::destroy() {
  m_clockTimer.stop();
  m_fingerprint.reset();
  for (auto& inst : m_instances) {
    inst->animations.cancelAll();
  }
  m_instances.clear();
  if (m_lock != nullptr) {
    ext_session_lock_v1_destroy(m_lock);
    m_lock = nullptr;
  }
  clearPassword();
}

void LockScreen::unlock() {
  m_clockTimer.stop();
  m_fingerprint.reset();
  for (auto& inst : m_instances) {
    inst->animations.cancelAll();
  }
  m_instances.clear();
  if (m_lock != nullptr) {
    ext_session_lock_v1_unlock_and_destroy(m_lock);
    m_lock = nullptr;
  }
  clearPassword();
  kLog.info("unlocked");
}

void LockScreen::clearPassword() {
  // Overwrite before releasing so the password doesn't linger in freed memory.
  std::fill(m_password.begin(), m_password.end(), '\0');
  m_password.clear();
}

void LockScreen::buildScene(Instance& inst) {
  const auto w = static_cast<float>(inst.surface->width());
  const auto h = static_cast<float>(inst.surface->height());
  Renderer& renderer = inst.surface->renderTarget().renderer();
  inst.animations.cancelAll();
  inst.root = std::make_unique<Node>();
  inst.root->setSize(w, h);

  auto bg = std::make_unique<Box>();
  bg->setSize(w, h);
  bg->setFill(theme::surface);
  inst.root->addChild(std::move(bg));
  if (!inst.content) {
    inst.surface->setSceneRoot(inst.root.get());
    return;
  }
  if (const std::string wallpaper = wallpaperPath(); !wallpaper.empty()) {
    auto image = std::make_unique<Image>();
    image->setSize(w, h);
    image->setFit(ImageFit::Cover);
    auto* img = static_cast<Image*>(inst.root->addChild(std::move(image)));
    img->setSourceFileAsync(renderer, *m_textures, wallpaper);
    Instance* raw = &inst;
    img->setAsyncReadyCallback([raw] { raw->surface->requestRedraw(); });
  }
  // Keeps the white text readable on bright wallpapers.
  auto dim = std::make_unique<Box>();
  dim->setSize(w, h);
  dim->setFill(Color{0.0F, 0.0F, 0.0F, 0.35F});
  inst.root->addChild(std::move(dim));

  inst.clock = widgets::label(*inst.root, renderer, "", theme::lockClockFontSize, theme::textPrimary, true);
  inst.date = widgets::label(*inst.root, renderer, "", theme::titleFontSize + 3.0F, theme::textPrimary);
  inst.date->setOpacity(0.85F);
  inst.hint = widgets::label(*inst.root, renderer, "", theme::bodyFontSize, theme::textSecondary);

  auto group = std::make_unique<Node>();
  group->setSize(kFieldW, kFieldH + 34.0F);
  group->setPosition((w - kFieldW) / 2.0F, h - kFieldMargin - kFieldH - 34.0F);
  inst.fieldGroup = inst.root->addChild(std::move(group));
  inst.error = widgets::label(*inst.fieldGroup, renderer, "", theme::bodyFontSize, theme::recording);
  auto field = std::make_unique<Box>();
  field->setPosition(0.0F, 34.0F);
  field->setSize(kFieldW, kFieldH);
  field->setRadius(kFieldH / 2.0F);
  field->setFill(theme::surface);
  field->setBorder(theme::divider, 1.0F);
  inst.field = static_cast<Box*>(inst.fieldGroup->addChild(std::move(field)));
  Glyph* lockIcon = widgets::glyph(*inst.field, renderer, "lock", 18.0F, theme::textSecondary);
  lockIcon->setPosition(20.0F, (kFieldH - lockIcon->height()) / 2.0F);
  inst.stars = widgets::label(*inst.field, renderer, "", theme::titleFontSize + 3.0F, theme::textPrimary, false,
                              kFieldW - 100.0F);

  inst.surface->setSceneRoot(inst.root.get());
  refresh(inst);
}

void LockScreen::refresh(Instance& inst) {
  if (inst.root == nullptr || !inst.content) {
    return;
  }
  Renderer& renderer = inst.surface->renderTarget().renderer();
  const float w = inst.root->width();
  const float h = inst.root->height();

  inst.clock->setText(formatNow(m_config->config().island.timeFormat.c_str()));
  inst.clock->measure(renderer);
  inst.clock->setPosition((w - inst.clock->width()) / 2.0F, h / 2.0F - inst.clock->height() + 10.0F);
  inst.date->setText(formatNow("%A, %-d %B"));
  inst.date->measure(renderer);
  inst.date->setPosition((w - inst.date->width()) / 2.0F, inst.clock->y() + inst.clock->height() + 4.0F);
  inst.hint->setText(m_hint);
  inst.hint->measure(renderer);
  inst.hint->setPosition((w - inst.hint->width()) / 2.0F, inst.date->y() + inst.date->height() + 18.0F);

  // The field shows up once the user starts typing (or while checking / after an error).
  const bool shown = !m_password.empty() || m_checking || !m_error.empty();
  inst.fieldGroup->setVisible(shown);
  inst.error->setText(m_error);
  inst.error->measure(renderer);
  inst.error->setPosition((kFieldW - inst.error->width()) / 2.0F, 6.0F);
  inst.field->setBorder(m_error.empty() ? theme::divider : theme::recording, 1.0F);
  std::string stars(utf8Length(m_password), '*');
  inst.stars->setText(m_checking ? std::string("Checking...") : stars);
  inst.stars->setColor(m_checking ? theme::textSecondary : theme::textPrimary);
  inst.stars->measure(renderer);
  inst.stars->setPosition((kFieldW - inst.stars->width()) / 2.0F, (kFieldH - inst.stars->height()) / 2.0F);
  inst.surface->requestRedraw();
}

void LockScreen::refreshAll() {
  for (auto& inst : m_instances) {
    refresh(*inst);
  }
}

void LockScreen::submit() {
  if (m_checking || m_password.empty()) {
    return;
  }
  m_checking = true;
  m_error.clear();
  refreshAll();
  const std::uint64_t generation = ++m_generation;
  std::string password = std::move(m_password);
  clearPassword();
  std::thread([this, generation, password = std::move(password)] mutable {
    PamAuthenticator authenticator;
    const auto result = authenticator.authenticateCurrentUser(password, "login", "en", "Could not start authentication");
    std::fill(password.begin(), password.end(), '\0');
    DeferredCall::callLater([this, generation, result] {
      if (generation != m_generation || m_lock == nullptr) {
        return;
      }
      m_checking = false;
      if (result.success) {
        unlock();
        return;
      }
      m_error = "Wrong password";
      refreshAll();
      // Shake the field.
      for (auto& inst : m_instances) {
        Box* field = inst->field;
        if (field == nullptr) {
          continue;
        }
        Instance* raw = inst.get();
        inst->animations.cancelForOwner(field);
        inst->animations.animate(
            0.0F, 1.0F, 420.0F, Easing::Linear,
            [field, raw](float t) {
              const float offset = 12.0F * std::sin(t * 6.0F * 3.14159265F) * (1.0F - t);
              field->setPosition(offset, field->y());
              raw->surface->requestRedraw();
            },
            [field] { field->setPosition(0.0F, field->y()); }, field
        );
      }
    });
  }).detach();
}

bool LockScreen::onKeyboardEvent(const KeyboardEvent& event) {
  if (!active()) {
    return false;
  }
  if (!event.pressed || m_checking) {
    return true;
  }
  switch (event.sym) {
  case XKB_KEY_Return:
  case XKB_KEY_KP_Enter:
    submit();
    return true;
  case XKB_KEY_Escape:
    clearPassword();
    m_error.clear();
    break;
  case XKB_KEY_BackSpace:
    while (!m_password.empty()) {
      const auto c = static_cast<unsigned char>(m_password.back());
      m_password.back() = '\0';
      m_password.pop_back();
      if ((c & 0xC0) != 0x80) {
        break;
      }
    }
    break;
  default:
    if (event.utf32 < 0x20 || event.utf32 == 0x7F || m_password.size() >= kMaxPassword) {
      return true;
    }
    appendUtf8(m_password, event.utf32);
    m_error.clear();
    break;
  }
  refreshAll();
  return true;
}

bool LockScreen::onPointerEvent(const PointerEvent& /*event*/) {
  // While locked nothing else may receive input.
  return active();
}
