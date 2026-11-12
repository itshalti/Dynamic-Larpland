// One tile of the power page. With `confirm` the first click arms it (red, "Confirm?")
// and only a second click within a few seconds runs the action.
import QtQuick
import qs.components

Rectangle {
    id: root

    property string icon
    property string label
    property bool confirm: false
    property bool armed: false

    signal triggered

    implicitWidth: 92
    implicitHeight: 92
    radius: Theme.radiusLarge
    color: armed ? Theme.recording : area.containsMouse ? Theme.surfaceHover : Theme.surfaceRaised
    scale: area.pressed ? 0.94 : 1

    Behavior on color {
        ColorAnimation {
            duration: Theme.animFast
        }
    }

    Behavior on scale {
        NumberAnimation {
            duration: Theme.animFast
        }
    }

    // Disarm when the page is left.
    onVisibleChanged: armed = false

    Column {
        anchors.centerIn: parent
        spacing: 8

        Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            name: root.icon
            size: 26
        }

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.armed ? "Confirm?" : root.label
            font.pixelSize: Theme.fontSmall
        }
    }

    Timer {
        running: root.armed
        interval: Theme.confirmTimeout
        onTriggered: root.armed = false
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            if (root.confirm && !root.armed) {
                root.armed = true;
                return;
            }
            root.armed = false;
            root.triggered();
        }
    }
}
