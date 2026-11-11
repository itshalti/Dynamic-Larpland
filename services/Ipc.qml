pragma Singleton
// Action entry points. Everything is callable over IPC:
//   qs -p <repo> ipc call island <action>      (or qs -c dynamic-larpland …)
// On Hyprland the same actions are also registered as global shortcuts
// (dynamic-larpland:<action>) which the generated binds.lua points at.
import Quickshell
import Quickshell.Io
import Quickshell.Hyprland
import QtQuick
import qs.services

Singleton {
    id: root

    signal toggleRequested
    signal collapseRequested
    signal modeRequested(string mode)
    signal launcherRequested

    IpcHandler {
        target: "island"

        function toggle(): void {
            root.toggleRequested();
        }

        function expand(): void {
            root.modeRequested("expanded");
        }

        function collapse(): void {
            root.collapseRequested();
        }

        function player(): void {
            root.modeRequested("player");
        }

        function calendar(): void {
            root.modeRequested("calendar");
        }

        function settings(): void {
            root.modeRequested("settings");
        }

        function launcher(): void {
            root.launcherRequested();
        }
    }

    LazyLoader {
        active: Compositor.isHyprland

        Scope {
            GlobalShortcut {
                appid: Compositor.appId
                name: "launcher"
                description: "Open the app launcher"
                onPressed: root.launcherRequested()
            }

            GlobalShortcut {
                appid: Compositor.appId
                name: "toggle"
                description: "Expand or collapse the island"
                onPressed: root.toggleRequested()
            }

            GlobalShortcut {
                appid: Compositor.appId
                name: "player"
                description: "Open the media player"
                onPressed: root.modeRequested("player")
            }

            GlobalShortcut {
                appid: Compositor.appId
                name: "calendar"
                description: "Open the calendar"
                onPressed: root.modeRequested("calendar")
            }
        }
    }
}
