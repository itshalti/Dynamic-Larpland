#pragma once

#include "dbus/mpris/mpris_service.h"

#include <sdbus-c++/sdbus-c++.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>

class HttpClient;
class SessionBus;

// Media for the island: the active MPRIS player, its cover art as a local file (remote art URLs are
// downloaded in the background) and its projected playback position.
class MediaService {
public:
  using ChangeCallback = std::function<void()>;

  MediaService(SessionBus& bus, HttpClient& http);

  MprisService& mpris() noexcept { return m_mpris; }
  std::optional<MprisPlayerInfo> active() const { return m_mpris.activePlayer(); }
  bool playing() const;
  // Local file for the active player's cover, or "" (none, or still downloading).
  std::string artPath();
  std::int64_t position() const;

  // Player list, track, playback state or cover download changed.
  void setChangeCallback(ChangeCallback callback) { m_onChange = std::move(callback); }

private:
  void notify();

  HttpClient& m_http;
  MprisService m_mpris;
  std::unordered_set<std::string> m_pendingArt;
  std::shared_ptr<int> m_lifetime = std::make_shared<int>(0);
  ChangeCallback m_onChange;
};
