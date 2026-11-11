pragma Singleton
// IPC entry point: `qs -c dynamic-larpland ipc call island <action>`.
// Islands listen to these signals; keybinds call the IPC functions.
import Quickshell
import Quickshell.Io
import QtQuick

Singleton {
    id: root

    signal toggleRequested
    signal expandRequested
    signal collapseRequested

    IpcHandler {
        target: "island"

        function toggle(): void {
            root.toggleRequested();
        }

        function expand(): void {
            root.expandRequested();
        }

        function collapse(): void {
            root.collapseRequested();
        }
    }
}
