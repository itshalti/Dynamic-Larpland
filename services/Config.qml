pragma Singleton
// User settings in ~/.config/dynamic-larpland/config.json. The JsonAdapter below holds the
// defaults; the file is created on first run and reloaded when edited by hand.
import Quickshell
import Quickshell.Io
import QtQuick

Singleton {
    id: root

    readonly property string dir: (Quickshell.env("XDG_CONFIG_HOME") || Quickshell.env("HOME") + "/.config") + "/dynamic-larpland"
    readonly property string stateDir: (Quickshell.env("XDG_STATE_HOME") || Quickshell.env("HOME") + "/.local/state") + "/dynamic-larpland"

    property alias island: adapter.island
    property alias calendar: adapter.calendar
    property alias notifications: adapter.notifications
    property alias keybinds: adapter.keybinds

    property bool dirsReady: false
    property bool fileMissing: false

    // Emitted after the config file was (re)loaded from disk.
    signal loaded

    Process {
        running: true
        command: ["mkdir", "-p", root.dir, root.stateDir]
        onExited: {
            root.dirsReady = true;
            if (root.fileMissing)
                file.writeAdapter();
        }
    }

    FileView {
        id: file

        path: root.dir + "/config.json"
        watchChanges: true
        printErrors: false
        onFileChanged: reload()
        onAdapterUpdated: writeAdapter()
        onLoaded: {
            root.fileMissing = false;
            root.loaded();
        }
        onLoadFailed: error => {
            if (error !== FileViewError.FileNotFound)
                return;
            root.fileMissing = true;
            if (root.dirsReady)
                writeAdapter();
        }

        JsonAdapter {
            id: adapter

            property JsonObject island: JsonObject {
                property bool hoverExpand: true
                property int hoverDelayMs: 150
                property string timeFormat: "HH:mm"
            }

            property JsonObject calendar: JsonObject {
                // 0 = Sunday, 1 = Monday
                property int firstDayOfWeek: 0
            }

            property JsonObject notifications: JsonObject {
                property int historyLimit: 100
                property int popupTimeoutMs: 4000
            }

            // Hyprland key combos ("SUPER + Space"); empty = unbound.
            property JsonObject keybinds: JsonObject {
                property string launcher: "SUPER + Space"
                property string toggle: ""
                property string player: ""
                property string calendar: ""
            }
        }
    }
}
