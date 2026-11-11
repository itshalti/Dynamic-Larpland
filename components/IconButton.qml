// Round (or rounded-square) icon button with hover and press feedback.
import QtQuick
import qs.components

Item {
    id: root

    property string icon
    property real iconSize: 16
    property real size: 32
    property bool round: true
    property color baseColor: "transparent"
    property color hoverColor: Theme.surfaceHover

    signal clicked

    implicitWidth: size
    implicitHeight: size
    opacity: enabled ? 1 : 0.35
    scale: area.pressed ? 0.88 : 1

    Behavior on scale {
        NumberAnimation {
            duration: Theme.animFast
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: root.round ? height / 2 : Theme.radiusSmall
        color: area.containsMouse ? root.hoverColor : root.baseColor

        Behavior on color {
            ColorAnimation {
                duration: Theme.animFast
            }
        }
    }

    Icon {
        anchors.centerIn: parent
        name: root.icon
        size: root.iconSize
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
