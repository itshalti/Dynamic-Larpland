pragma Singleton
// Session / power actions for the power page. Lock shows the island's own lock screen.
import Quickshell
import QtQuick
import qs.services

Singleton {
    readonly property var commands: ({
            poweroff: ["systemctl", "poweroff"],
            reboot: ["systemctl", "reboot"],
            firmware: ["systemctl", "reboot", "--firmware-setup"],
            suspend: ["systemctl", "suspend"]
        })

    function run(action: string): void {
        if (action === "lock")
            Lock.lock();
        else
            Quickshell.execDetached(commands[action]);
    }
}
