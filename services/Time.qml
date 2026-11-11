pragma Singleton
// Current date/time, updated every minute.
import Quickshell
import QtQuick
import qs.services

Singleton {
    readonly property date now: clock.date
    readonly property string time: Qt.formatDateTime(clock.date, Config.island.timeFormat)

    SystemClock {
        id: clock
        precision: SystemClock.Minutes
    }
}
