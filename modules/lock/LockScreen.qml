// Session lock (ext-session-lock): one LockSurface per screen while Lock.locked.
import Quickshell
import Quickshell.Wayland
import QtQuick
import qs.components
import qs.services

WlSessionLock {
    locked: Lock.locked

    WlSessionLockSurface {
        color: Theme.surface

        LockSurface {
            anchors.fill: parent
        }
    }
}
