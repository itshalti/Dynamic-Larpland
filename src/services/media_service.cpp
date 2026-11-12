#include "services/media_service.h"

#include "dbus/mpris/mpris_art.h"

MediaService::MediaService(SessionBus& bus, HttpClient& http) : m_http(http), m_mpris(bus) {
  m_mpris.setChangeCallback([this] { notify(); });
}

bool MediaService::playing() const {
  const auto player = active();
  return player && player->playbackStatus == "Playing";
}

std::string MediaService::artPath() {
  const auto player = active();
  if (!player) {
    return {};
  }
  return mpris::resolveArtworkSource(
      &m_http, m_pendingArt, mpris::effectiveArtUrl(*player), [this] { notify(); }, m_lifetime
  );
}

std::int64_t MediaService::position() const {
  const auto player = active();
  if (!player) {
    return 0;
  }
  return m_mpris.position(player->busName).value_or(player->positionUs);
}

void MediaService::notify() {
  if (m_onChange) {
    m_onChange();
  }
}
