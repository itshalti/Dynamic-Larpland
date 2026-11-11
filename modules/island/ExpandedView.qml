// Expanded content. Placeholder until the Nook/Tray tabs, media card, week strip
// and notification list are implemented (see CLAUDE.md).
import QtQuick
import qs.components
import qs.services

Item {
    Column {
        anchors.centerIn: parent
        spacing: 6

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Time.time
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontTitle * 2
            font.weight: Font.Bold
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Qt.formatDateTime(Time.now, "dddd, d MMMM")
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
        }
    }
}
