#include "settings/settings_registry.h"

namespace settings {

  const std::vector<Section>& sections() {
    static const std::vector<Section> list = {
        {"appearance", "Appearance", "palette"},
        {"wallpaper", "Wallpaper", "photo"},
        {"templates", "Templates", "brush"},
        {"island", "Island", "layout-navbar"},
        {"keybinds", "Keybinds", "keyboard"},
        {"notifications", "Notifications", "bell"},
        {"calendar", "Calendar", "calendar"},
        {"recorder", "Recorder", "video"},
        {"power", "Power", "power"},
    };
    return list;
  }

  const std::vector<Setting>& registry() {
    static const std::vector<Setting> list = {
        // Appearance
        {.section = "appearance", .group = "Colors", .path = "theme.scheme", .title = "Color scheme",
         .description = "How the palette is built from the wallpaper.", .kind = Kind::Select,
         .options = {{"m3-tonal-spot", "Tonal spot"}, {"m3-content", "Content"}, {"m3-fruit-salad", "Fruit salad"},
                     {"m3-rainbow", "Rainbow"}, {"m3-monochrome", "Monochrome"}, {"vibrant", "Vibrant"},
                     {"faithful", "Faithful"}, {"soft", "Soft"}, {"dysfunctional", "Dysfunctional"},
                     {"muted", "Muted"}}},
        {.section = "appearance", .group = "Colors", .path = "theme.mode", .title = "Mode",
         .description = "Palette variant used for the island accent and the app templates.", .kind = Kind::Select,
         .options = {{"dark", "Dark"}, {"light", "Light"}}},

        // Wallpaper
        {.section = "wallpaper", .group = "Wallpaper", .path = "wallpaper.dir", .title = "Wallpaper folder",
         .description = "Images shown in the island's wallpaper picker.", .kind = Kind::Text},
        {.section = "wallpaper", .group = "Wallpaper", .path = "wallpaper.current", .title = "Current wallpaper",
         .kind = Kind::Custom, .id = "current-wallpaper"},

        // Templates
        {.section = "templates", .group = "Built-in", .path = "theme.builtin_templates",
         .title = "Enable built-in templates", .description = "Recolor the selected apps on every wallpaper change.",
         .kind = Kind::Toggle},
        {.section = "templates", .group = "Built-in", .path = "theme.templates", .title = "Built-in templates",
         .description = "Apps recolored from the wallpaper palette.", .kind = Kind::Custom, .id = "template-grid"},

        // Island
        {.section = "island", .group = "Behaviour", .path = "island.hover_expand", .title = "Expand on hover",
         .description = "Open the island when the pointer rests on it.", .kind = Kind::Toggle},
        {.section = "island", .group = "Behaviour", .path = "island.hover_delay_ms", .title = "Hover delay",
         .description = "How long the pointer must rest on the pill before it opens.", .kind = Kind::Slider,
         .advanced = true, .min = 0, .max = 1000, .step = 25, .unitFormat = "{} ms"},
        {.section = "island", .group = "Clock", .path = "island.time_format", .title = "Clock format",
         .kind = Kind::Select,
         .options = {{"%H:%M", "24-hour (23:59)"}, {"%H:%M:%S", "24-hour with seconds"},
                     {"%I:%M %p", "12-hour (11:59 PM)"}}},

        // Keybinds
        {.section = "keybinds", .group = "Keybinds", .path = "keybinds", .title = "Keybinds",
         .description = "Shortcuts for the island's actions.", .kind = Kind::Custom, .id = "keybinds"},

        // Notifications
        {.section = "notifications", .group = "Popups", .path = "notifications.dnd", .title = "Do not disturb",
         .description = "No popups; everything still goes to the history. Critical notifications still show.",
         .kind = Kind::Toggle},
        {.section = "notifications", .group = "Popups", .path = "notifications.popup_timeout_ms",
         .title = "Popup duration", .description = "How long a new notification shows in the pill.",
         .kind = Kind::Slider, .min = 1000, .max = 15000, .step = 500, .unitFormat = "{:.1f} s",
         .displayScale = 1000.0},
        {.section = "notifications", .group = "History", .path = "notifications.history_limit",
         .title = "History size", .description = "How many notifications are kept.", .kind = Kind::Slider,
         .advanced = true, .min = 10, .max = 500, .step = 10},
        {.section = "notifications", .group = "History", .title = "Notification history", .kind = Kind::Custom,
         .id = "clear-history"},
        {.section = "notifications", .group = "App rules", .path = "notifications.rules", .title = "App rules",
         .description = "Mute (history only) or block (dropped) notifications per app.", .kind = Kind::Custom,
         .id = "notification-rules"},

        // Calendar
        {.section = "calendar", .group = "Calendar", .path = "calendar.first_day_of_week",
         .title = "First day of the week", .kind = Kind::Select, .options = {{"0", "Sunday"}, {"1", "Monday"}},
         .integerValue = true},

        // Recorder
        {.section = "recorder", .group = "Screen recorder", .path = "recorder.dir", .title = "Recordings folder",
         .kind = Kind::Text},
        {.section = "recorder", .group = "Screen recorder", .path = "recorder.fps", .title = "Frame rate",
         .kind = Kind::Select, .advanced = true,
         .options = {{"30", "30"}, {"60", "60"}, {"120", "120"}, {"144", "144"}, {"165", "165"}, {"240", "240"}},
         .integerValue = true},
        {.section = "recorder", .group = "Screen recorder", .path = "recorder.audio", .title = "Audio",
         .kind = Kind::Select,
         .options = {{"default_output", "Desktop audio"}, {"default_input", "Microphone"},
                     {"default_output|default_input", "Desktop + microphone"}, {"", "No audio"}}},

        // Power
        {.section = "power", .group = "Power", .title = "Power actions", .kind = Kind::Custom, .id = "power-info"},
    };
    return list;
  }

} // namespace settings
