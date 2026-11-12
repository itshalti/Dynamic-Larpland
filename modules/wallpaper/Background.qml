// Desktop wallpaper for one screen (layer Background), cross-fading to a newly picked image.
import Quickshell
import Quickshell.Wayland
import QtQuick
import qs.components
import qs.services

PanelWindow {
    id: root

    required property ShellScreen modelData

    screen: modelData
    visible: Wallpaper.current !== ""
    anchors {
        top: true
        bottom: true
        left: true
        right: true
    }
    color: "transparent"
    exclusionMode: ExclusionMode.Ignore
    WlrLayershell.layer: WlrLayer.Background
    WlrLayershell.namespace: "dynamic-larpland-wallpaper"

    // The previous wallpaper stays underneath while the new one fades in.
    Image {
        id: previous
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        sourceSize: Qt.size(root.width, root.height)
    }

    Image {
        id: next
        anchors.fill: parent
        source: Wallpaper.current ? "file://" + Wallpaper.current : ""
        fillMode: Image.PreserveAspectCrop
        sourceSize: Qt.size(root.width, root.height)
        asynchronous: true
        opacity: 0
        onSourceChanged: opacity = 0
        onStatusChanged: if (status === Image.Ready)
            fade.restart()

        NumberAnimation {
            id: fade
            target: next
            property: "opacity"
            to: 1
            duration: Theme.animWallpaper
            easing.type: Easing.OutQuad
            onFinished: previous.source = next.source
        }
    }
}
