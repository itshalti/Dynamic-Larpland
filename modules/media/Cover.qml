// Album art in a rounded tile; a music note when there's no art.
import QtQuick
import Quickshell.Widgets
import qs.components

ClippingRectangle {
    id: root

    property string source
    property real size: 64

    implicitWidth: size
    implicitHeight: size
    radius: size * 0.16
    color: Theme.surfaceRaised

    Image {
        id: art
        anchors.fill: parent
        source: root.source
        sourceSize: Qt.size(root.size * 2, root.size * 2)
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        opacity: status === Image.Ready ? 1 : 0

        Behavior on opacity {
            NumberAnimation {
                duration: Theme.animFade
            }
        }
    }

    Icon {
        anchors.centerIn: parent
        visible: art.status !== Image.Ready
        name: "music"
        size: root.size * 0.4
        opacity: 0.4
    }
}
