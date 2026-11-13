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
        {"lockscreen", "Lock screen", "lock"},
        {"location", "Location", "map-pin"},
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
        {.section = "appearance", .group = "Colors", .path = "theme.auto_mode", .title = "Switch automatically",
         .description = "Light from sunrise to sunset, dark at night (set your location).", .kind = Kind::Toggle},

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

        {.section = "templates", .group = "Community", .path = "theme.community_templates",
         .title = "Enable community templates",
         .description = "Templates made by the community (Noctalia's catalog), downloaded when selected.",
         .kind = Kind::Toggle},
        {.section = "templates", .group = "Community", .path = "theme.community", .title = "Community templates",
         .description = "Apps recolored by community templates.", .kind = Kind::Custom, .id = "community-grid"},

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

        // Lock screen
        {.section = "lockscreen", .group = "Lock screen", .path = "lockscreen.wallpaper", .title = "Lock wallpaper",
         .description = "Image path; empty uses the desktop wallpaper.", .kind = Kind::Text},
        {.section = "lockscreen", .group = "Lock screen", .path = "lockscreen.monitors", .title = "Show on",
         .description = "Monitors with the clock and the password field; the others stay black.",
         .kind = Kind::Select, .options = {{"all", "All monitors"}, {"focused", "Focused monitor"}}},
        {.section = "lockscreen", .group = "Lock screen", .path = "lockscreen.dnd", .title = "Do not disturb while locked",
         .description = "No notification popups while the screen is locked.", .kind = Kind::Toggle},
        {.section = "lockscreen", .group = "Unlock", .path = "lockscreen.fingerprint", .title = "Fingerprint",
         .description = "Also unlock with the fingerprint reader (needs fprintd and an enrolled finger).",
         .kind = Kind::Toggle},

        // Location
        {.section = "location", .group = "Location", .path = "location.latitude", .title = "Latitude",
         .description = "Decimal degrees, north positive (52.52).", .kind = Kind::Text, .numberValue = true},
        {.section = "location", .group = "Location", .path = "location.longitude", .title = "Longitude",
         .description = "Decimal degrees, east positive (13.40).", .kind = Kind::Text, .numberValue = true},
        {.section = "location", .group = "Location", .title = "Day and night", .kind = Kind::Custom,
         .id = "sun-times"},
        {.section = "location", .group = "Fixed schedule", .path = "location.custom_schedule",
         .title = "Use fixed times", .description = "Ignore the location and switch at the times below.",
         .kind = Kind::Toggle, .advanced = true},
        {.section = "location", .group = "Fixed schedule", .path = "location.sunrise", .title = "Day starts at",
         .description = "HH:MM", .kind = Kind::Text, .advanced = true},
        {.section = "location", .group = "Fixed schedule", .path = "location.sunset", .title = "Night starts at",
         .description = "HH:MM", .kind = Kind::Text, .advanced = true},

        // Power
        {.section = "power", .group = "When idle", .path = "idle.lock_enabled", .title = "Lock the screen",
         .description = "Lock after a while without keyboard or mouse input.", .kind = Kind::Toggle},
        {.section = "power", .group = "When idle", .path = "idle.lock_timeout_s", .title = "Lock after",
         .kind = Kind::Slider, .min = 60, .max = 3600, .step = 60, .unitFormat = "{:.0f} min",
         .displayScale = 60.0},
        {.section = "power", .group = "When idle", .path = "idle.screen_off_enabled", .title = "Turn screens off",
         .description = "They come back on with the next input (Hyprland).", .kind = Kind::Toggle},
        {.section = "power", .group = "When idle", .path = "idle.screen_off_timeout_s", .title = "Screens off after",
         .kind = Kind::Slider, .min = 60, .max = 3600, .step = 60, .unitFormat = "{:.0f} min",
         .displayScale = 60.0},
        {.section = "power", .group = "When idle", .path = "idle.suspend_enabled", .title = "Lock and suspend",
         .kind = Kind::Toggle},
        {.section = "power", .group = "When idle", .path = "idle.suspend_timeout_s", .title = "Suspend after",
         .kind = Kind::Slider, .min = 300, .max = 7200, .step = 300, .unitFormat = "{:.0f} min",
         .displayScale = 60.0},
        {.section = "power", .group = "When idle", .path = "idle.inhibit_while_playing",
         .title = "Stay awake while media plays", .kind = Kind::Toggle, .advanced = true},
        {.section = "power", .group = "Power", .title = "Power actions", .kind = Kind::Custom, .id = "power-info"},
    };
    return list;
  }

} // namespace settings
