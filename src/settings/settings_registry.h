#pragma once

#include <string>
#include <vector>

// The settings window's content as data, like Noctalia's settings registry: each row is a config path with a
// control kind. Search, the Advanced/Overridden filters and Reset page all work on this list.
namespace settings {

  struct Section {
    std::string id;
    std::string title;
    std::string glyph;
  };

  enum class Kind {
    Toggle,
    Select,
    Slider,
    Text,
    // Hand-written block (template grid, keybinds, ...), drawn by the window by `id`.
    Custom,
  };

  struct Option {
    std::string value;
    std::string label;
  };

  struct Setting {
    std::string section;
    // Card the row sits in.
    std::string group;
    // Config path ("island.hover_expand"); for Custom rows the path Reset/Overridden apply to ("" = none).
    std::string path;
    std::string title;
    std::string description;
    Kind kind = Kind::Toggle;
    bool advanced = false;
    // Select
    std::vector<Option> options;
    // Select values stored as integers in the config; Text values stored as numbers ("" removes the value).
    bool integerValue = false;
    bool numberValue = false;
    // Slider: value range, step and how the value is shown ("{} ms"; divided by displayScale first).
    double min = 0.0;
    double max = 0.0;
    double step = 1.0;
    std::string unitFormat = "{}";
    double displayScale = 1.0;
    // Custom block id.
    std::string id;
  };

  const std::vector<Section>& sections();
  const std::vector<Setting>& registry();

} // namespace settings
