#pragma once

#include "render/scene/node.h"

#include <functional>
#include <memory>
#include <string>

class AnimationManager;
class AsyncTextureCache;
class MediaService;
class IconResolver;
class NotificationService;
class RecorderService;
class TrayService;
class Renderer;
class WallpaperService;

// What an island page needs to build itself and to talk to the services.
struct PageContext {
  Renderer& renderer;
  AnimationManager& animations;
  AsyncTextureCache& textures;
  WallpaperService& wallpapers;
  MediaService& media;
  NotificationService& notifications;
  TrayService& tray;
  RecorderService& recorder;
  IconResolver& icons;
  std::function<void()> requestRedraw;
  // Runs an island/app action: a mode name ("player", "expanded", ...), "rebuild:<mode>" (rebuild that page
  // from current state) or an app action ("record", "settings").
  std::function<void(const std::string&)> action;
  // Opens a tray item's menu anchored at a rect in page coordinates.
  std::function<void(const std::string& itemId, float x, float y, float w, float h)> openTrayMenu;
  // 0 = Sunday, 1 = Monday (calendar.first_day_of_week).
  int firstDayOfWeek = 0;
};

// A built page: its scene subtree and an optional tick run a few times a second while it is shown
// (live values such as the playback position).
struct BuiltPage {
  std::unique_ptr<Node> node;
  std::function<void()> tick;
};
