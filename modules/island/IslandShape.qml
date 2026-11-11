// The black pill. Size and corner radius morph together toward their targets.
import QtQuick
import qs.components

Rectangle {
    id: root

    property size targetSize: Theme.collapsedSize
    property real targetRadius: Theme.collapsedRadius

    width: targetSize.width
    height: targetSize.height
    radius: targetRadius
    color: Theme.surface
    clip: true

    Behavior on width {
        NumberAnimation {
            duration: Theme.animMorph
            easing.type: Theme.easingMorph
            easing.overshoot: Theme.easingMorphOvershoot
        }
    }

    Behavior on height {
        NumberAnimation {
            duration: Theme.animMorph
            easing.type: Theme.easingMorph
            easing.overshoot: Theme.easingMorphOvershoot
        }
    }

    Behavior on radius {
        NumberAnimation {
            duration: Theme.animMorph
            easing.type: Easing.OutCubic
        }
    }
}
