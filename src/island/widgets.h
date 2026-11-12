#pragma once

#include "render/core/color.h"
#include "render/scene/input_area.h"

#include <functional>
#include <string>
#include <string_view>

class Box;
class Glyph;
class Label;
class Node;
class Renderer;

// Small building blocks shared by the island pages.
namespace widgets {

  Label* label(
      Node& parent, Renderer& renderer, const std::string& text, float size, const Color& color, bool bold = false,
      float maxWidth = 0.0F
  );

  Glyph* glyph(Node& parent, Renderer& renderer, std::string_view name, float size, const Color& color);

  // Round icon button: hover background, pointer cursor, click callback.
  InputArea* iconButton(
      Node& parent, Renderer& renderer, std::string_view glyphName, float size, float glyphSize,
      std::function<void()> onClick, std::function<void()> redraw, const Color& color = Color{1, 1, 1, 1}
  );

  // A clickable area with the pointer cursor.
  InputArea* clickArea(Node& parent, float x, float y, float w, float h, std::function<void()> onClick);

  // mm:ss (or h:mm:ss) for media times.
  std::string formatDuration(std::int64_t microseconds);

} // namespace widgets
