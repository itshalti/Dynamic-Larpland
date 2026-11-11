// Pill-shaped tab button (icon + label) for the expanded island header.
import QtQuick
import qs.components

Rectangle {
    id: tabButton

    property string icon
    property string label
    property bool selected

    signal clicked

    implicitWidth: tabRow.implicitWidth + 26
    implicitHeight: 34
    radius: height / 2
    color: selected ? Theme.surfaceRaised : tabArea.containsMouse ? "#111113" : "transparent"

    Behavior on color {
        ColorAnimation {
            duration: Theme.animFast
        }
    }

    Row {
        id: tabRow
        anchors.centerIn: parent
        spacing: 7

        Icon {
            anchors.verticalCenter: parent.verticalCenter
            name: tabButton.icon
            size: 16
        }

        StyledText {
            anchors.verticalCenter: parent.verticalCenter
            text: tabButton.label
            font.pixelSize: Theme.fontLarge
            font.weight: Font.DemiBold
        }
    }

    MouseArea {
        id: tabArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: tabButton.clicked()
    }
}
