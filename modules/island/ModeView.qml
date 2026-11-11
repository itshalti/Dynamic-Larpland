// A page of the island for one mode. Laid out at its final size (so it doesn't reflow while
// the pill morphs) and cross-faded in/out.
import QtQuick
import qs.components

Item {
    id: root

    property bool active: false
    property size pageSize

    anchors.top: parent.top
    anchors.horizontalCenter: parent.horizontalCenter
    width: pageSize.width
    height: pageSize.height
    opacity: active ? 1 : 0
    visible: opacity > 0
    enabled: active

    Behavior on opacity {
        NumberAnimation {
            duration: root.active ? Theme.animFade * 1.6 : Theme.animFade
            easing.type: Easing.OutQuad
        }
    }
}
