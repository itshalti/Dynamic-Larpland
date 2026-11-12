// One wallpaper preview in the picker: rounded thumbnail + file name; the current one is outlined.
import QtQuick
import Quickshell.Widgets
import qs.components

Item {
    id: root

    property string path
    property string name
    property bool current: false

    signal picked

    implicitWidth: 200
    implicitHeight: thumb.height + 26
    scale: area.pressed ? 0.95 : area.containsMouse ? 1.04 : 1

    Behavior on scale {
        NumberAnimation {
            duration: Theme.animFast
        }
    }

    ClippingRectangle {
        id: thumb
        width: parent.width
        height: 112
        radius: Theme.radiusMedium
        color: Theme.surfaceRaised
        border.width: root.current ? 2 : 0
        border.color: Theme.accent

        Image {
            anchors.fill: parent
            source: "file://" + root.path
            // Decode a small preview, not the full-size wallpaper.
            sourceSize: Qt.size(400, 224)
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
        }
    }

    StyledText {
        anchors.top: thumb.bottom
        anchors.topMargin: 6
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        text: root.name.replace(/\.[^.]+$/, "")
        color: root.current ? Theme.textPrimary : Theme.textSecondary
        font.pixelSize: Theme.fontSmall
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.picked()
    }
}
