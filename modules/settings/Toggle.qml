// iOS-style on/off switch.
import QtQuick
import qs.components

Rectangle {
    id: sw

    property bool checked

    signal toggled

    implicitWidth: 40
    implicitHeight: 24
    radius: height / 2
    color: checked ? Theme.accent : Theme.track

    Behavior on color {
        ColorAnimation {
            duration: Theme.animFast
        }
    }

    Rectangle {
        x: sw.checked ? sw.width - width - 3 : 3
        anchors.verticalCenter: parent.verticalCenter
        width: 18
        height: 18
        radius: 9
        color: "#ffffff"

        Behavior on x {
            NumberAnimation {
                duration: Theme.animFast
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: sw.toggled()
    }
}
