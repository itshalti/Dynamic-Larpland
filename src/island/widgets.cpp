#include "island/widgets.h"

#include "cursor-shape-v1-client-protocol.h"
#include "island/theme.h"
#include "ui/controls/box.h"
#include "ui/controls/glyph.h"
#include "ui/controls/label.h"

#include <format>

namespace widgets {

  Label* label(
      Node& parent, Renderer& renderer, const std::string& text, float size, const Color& color, bool bold,
      float maxWidth
  ) {
    auto node = std::make_unique<Label>();
    node->setFontSize(size);
    node->setColor(color);
    if (bold) {
      node->setFontWeight(FontWeight::Bold);
    }
    if (maxWidth > 0.0F) {
      node->setMaxWidth(maxWidth);
    }
    node->setText(text);
    node->measure(renderer);
    return static_cast<Label*>(parent.addChild(std::move(node)));
  }

  Glyph* glyph(Node& parent, Renderer& renderer, std::string_view name, float size, const Color& color) {
    auto node = std::make_unique<Glyph>();
    node->setGlyph(name);
    node->setGlyphSize(size);
    node->setColor(color);
    node->measure(renderer);
    return static_cast<Glyph*>(parent.addChild(std::move(node)));
  }

  InputArea* iconButton(
      Node& parent, Renderer& renderer, std::string_view glyphName, float size, float glyphSize,
      std::function<void()> onClick, std::function<void()> redraw, const Color& color
  ) {
    auto area = std::make_unique<InputArea>();
    area->setSize(size, size);
    area->setCursorShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER);
    auto* button = static_cast<InputArea*>(parent.addChild(std::move(area)));

    auto bg = std::make_unique<Box>();
    bg->setSize(size, size);
    bg->setRadius(size / 2.0F);
    bg->setFill(Color{0, 0, 0, 0});
    auto* background = static_cast<Box*>(button->addChild(std::move(bg)));

    Glyph* icon = glyph(*button, renderer, glyphName, glyphSize, color);
    icon->setPosition((size - icon->width()) / 2.0F, (size - icon->height()) / 2.0F);

    button->setOnEnter([background, redraw](const InputArea::PointerData&) {
      background->setFill(theme::surfaceHover);
      redraw();
    });
    button->setOnLeave([background, redraw] {
      background->setFill(Color{0, 0, 0, 0});
      redraw();
    });
    button->setOnClick([onClick = std::move(onClick)](const InputArea::PointerData&) { onClick(); });
    return button;
  }

  InputArea* clickArea(Node& parent, float x, float y, float w, float h, std::function<void()> onClick) {
    auto area = std::make_unique<InputArea>();
    area->setPosition(x, y);
    area->setSize(w, h);
    area->setCursorShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER);
    area->setOnClick([onClick = std::move(onClick)](const InputArea::PointerData&) { onClick(); });
    return static_cast<InputArea*>(parent.addChild(std::move(area)));
  }

  std::string formatDuration(std::int64_t microseconds) {
    const std::int64_t total = std::max<std::int64_t>(0, microseconds / 1'000'000);
    if (total >= 3600) {
      return std::format("{}:{:02}:{:02}", total / 3600, (total / 60) % 60, total % 60);
    }
    return std::format("{}:{:02}", total / 60, total % 60);
  }

} // namespace widgets
