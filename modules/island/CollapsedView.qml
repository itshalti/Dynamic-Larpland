// Collapsed content: just the clock (live activities come later).
import QtQuick
import qs.components
import qs.services

Item {
    Text {
        anchors.centerIn: parent
        text: Time.time
        color: Theme.textPrimary
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontClock
        font.weight: Font.DemiBold
    }
}
