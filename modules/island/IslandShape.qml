// The black pill. Size and corner radius morph together toward their targets;
// content is clipped to the rounded shape.
import QtQuick
import Quickshell.Widgets
import qs.components

ClippingRectangle {
    id: root

    property size targetSize: Theme.collapsedSize
    property real targetRadius: targetSize.height / 2

    width: targetSize.width
    height: targetSize.height
    radius: targetRadius
    color: Theme.surface

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
