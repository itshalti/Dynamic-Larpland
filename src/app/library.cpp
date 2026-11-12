#include "app/library.h"

#include "core/application_identity.h"
#include "core/files/resource_paths.h"
#include "core/ui_strings.h"
#include "render/codecs/codecs.h"

#include <string>
#include <string_view>

namespace {

  std::string keyNameLabel(std::string_view name) {
    if (name == "Return" || name == "KP_Enter") {
      return "Enter";
    }
    if (name == "space") {
      return "Space";
    }
    if (name == "Prior") {
      return "Page Up";
    }
    if (name == "Next") {
      return "Page Down";
    }
    if (name == "ISO_Left_Tab") {
      return "Tab";
    }
    if (name == "BackSpace") {
      return "Backspace";
    }
    if (name == "Escape" || name == "Tab" || name == "Delete" || name == "Insert" || name == "Home" || name == "End"
        || name == "Left" || name == "Right" || name == "Up" || name == "Down") {
      return std::string(name);
    }
    return {};
  }

} // namespace

void configureLibrary() {
  setApplicationIdentity({.name = "dynamic-larpland", .envPrefix = "DYNAMIC_LARPLAND"});
  paths::setAssetRoots({.application = DL_APP_ASSETS_DIR, .library = DL_LIB_ASSETS_DIR});
  setUiStrings({
      .keyName = keyNameLabel,
      .selectPlaceholder = [] { return std::string("Select"); },
  });
  codecs::installImageDecoder();
}
