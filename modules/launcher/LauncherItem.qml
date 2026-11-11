// One app row: icon tile, name, generic name; the selected row is highlighted with an accent bar.
import QtQuick
import QtQuick.Layouts
import Quickshell.Widgets
import qs.components
import qs.services

Rectangle {
    id: root

    required property var entry
    property bool selected: false

    signal activated
    signal hovered

    implicitHeight: Theme.launcherRowHeight
    radius: Theme.radiusMedium
    color: selected ? Theme.surfaceRaised : "transparent"

    Behavior on color {
        ColorAnimation {
            duration: Theme.animFast
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.leftMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        width: 3
        height: 26
        radius: 1.5
        color: Theme.accent
        opacity: root.selected ? 1 : 0

        Behavior on opacity {
            NumberAnimation {
                duration: Theme.animFast
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 14

        Rectangle {
            Layout.preferredWidth: 42
            Layout.preferredHeight: 42
            radius: 11
            color: "#141416"

            IconImage {
                anchors.centerIn: parent
                implicitSize: 28
                source: Apps.icon(root.entry)
                asynchronous: true
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 1

            StyledText {
                Layout.fillWidth: true
                text: root.entry.name
                font.pixelSize: Theme.fontLarge
                font.weight: Font.DemiBold
            }

            StyledText {
                Layout.fillWidth: true
                visible: text !== ""
                text: Apps.subtitle(root.entry)
                color: Theme.textSecondary
                font.pixelSize: 13
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onEntered: root.hovered()
        onClicked: root.activated()
    }
}
