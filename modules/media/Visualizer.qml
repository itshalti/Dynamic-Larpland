// Equalizer-style bars. Animated pseudo-randomly while playing (cava support can come later).
import QtQuick
import qs.components

Row {
    id: root

    property bool playing: false
    property int bars: 5
    property real barWidth: 3
    property real maxHeight: 24
    property color color: Theme.textSecondary
    property var levels: []

    height: maxHeight
    spacing: barWidth * 0.8

    function randomize(): void {
        const next = [];
        for (let i = 0; i < bars; i++)
            next.push(0.25 + Math.random() * 0.75);
        levels = next;
    }

    Repeater {
        model: root.bars

        Rectangle {
            required property int index

            anchors.verticalCenter: parent.verticalCenter
            width: root.barWidth
            height: root.playing ? Math.max(root.barWidth, (root.levels[index] ?? 0.3) * root.maxHeight) : root.barWidth
            radius: width / 2
            color: root.color

            Behavior on height {
                NumberAnimation {
                    duration: 170
                    easing.type: Easing.OutQuad
                }
            }
        }
    }

    Timer {
        running: root.playing && root.visible
        interval: 170
        repeat: true
        triggeredOnStart: true
        onTriggered: root.randomize()
    }
}
