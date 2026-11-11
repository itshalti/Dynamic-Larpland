// An icon from a path/URL/theme lookup in a rounded tile, with a generic fallback.
import QtQuick
import qs.components
import Quickshell.Widgets

ClippingRectangle {
    id: root

    property string source
    property real size: 32
    property real iconScale: 1
    readonly property bool ready: img.status === Image.Ready

    implicitWidth: size
    implicitHeight: size
    radius: size * 0.25
    color: img.status === Image.Ready ? "transparent" : Theme.surfaceRaised

    Image {
        id: img
        anchors.centerIn: parent
        width: root.size * root.iconScale
        height: width
        sourceSize: Qt.size(width * 2, height * 2)
        source: root.source
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        smooth: true
    }

    Icon {
        anchors.centerIn: parent
        visible: img.status !== Image.Ready
        name: "app"
        size: root.size * 0.55
        opacity: 0.5
    }
}
