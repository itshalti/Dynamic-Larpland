pragma Singleton
// Wallpapers: the images in Config.wallpaper.dir, the current one (drawn by modules/wallpaper/Background)
// and the colors matugen extracts from it — the island accent, GTK, kitty and Hyprland borders.
import Quickshell
import Quickshell.Io
import QtQuick
import Qt.labs.folderlistmodel
import qs.services

Singleton {
    id: root

    readonly property string dir: Config.expandHome(Config.wallpaper.dir)
    readonly property string current: Config.wallpaper.current
    readonly property alias images: folder

    // Island colors from the last matugen run; "" until there is one.
    readonly property string accent: colors.accent
    readonly property string textOnAccent: colors.textOnAccent

    readonly property string configHome: Quickshell.env("XDG_CONFIG_HOME") || Quickshell.env("HOME") + "/.config"
    readonly property string templates: Quickshell.shellDir + "/assets/matugen"

    function set(path: string): void {
        Config.wallpaper.current = path;
        matugenConfig.setText(`[config]

[templates.island]
input_path = "${templates}/island.json"
output_path = "${Config.stateDir}/colors.json"

[templates.gtk3]
input_path = "${templates}/gtk.css"
output_path = "${configHome}/gtk-3.0/dynamic-larpland.css"

[templates.gtk4]
input_path = "${templates}/gtk.css"
output_path = "${configHome}/gtk-4.0/dynamic-larpland.css"

[templates.kitty]
input_path = "${templates}/kitty.conf"
output_path = "${configHome}/kitty/themes/dynamic-larpland.conf"

[templates.hyprland]
input_path = "${templates}/hyprland.lua"
output_path = "${Config.dir}/colors.lua"
`);
        matugen.command = ["sh", "-c", 'matugen image "$1" -c "$2" -m dark --source-color-index 0 && "$3"', "sh", path, matugenConfig.path, Quickshell.shellDir + "/scripts/apply-colors.sh"];
        matugen.running = false;
        matugen.running = true;
    }

    FolderListModel {
        id: folder
        folder: "file://" + root.dir
        nameFilters: ["*.jpg", "*.jpeg", "*.png", "*.webp", "*.bmp"]
        caseSensitive: false
        showDirs: false
        sortField: FolderListModel.Name
    }

    FileView {
        id: matugenConfig
        path: Config.dir + "/matugen.toml"
        // matugen reads this file right after set() writes it.
        blockWrites: true
        printErrors: false
    }

    Process {
        id: matugen
        stderr: StdioCollector {
            id: err
        }
        onExited: code => {
            if (code !== 0)
                console.warn("matugen failed:", code, err.text);
        }
    }

    FileView {
        path: Config.stateDir + "/colors.json"
        watchChanges: true
        printErrors: false
        onFileChanged: reload()

        JsonAdapter {
            id: colors
            property string accent: ""
            property string textOnAccent: ""
        }
    }
}
