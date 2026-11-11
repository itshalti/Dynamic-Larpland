pragma Singleton
// Design tokens. Modules must take every color, size, radius and timing from here.
import Quickshell
import QtQuick

Singleton {
    // Colors
    readonly property color surface: "#000000"
    readonly property color surfaceRaised: "#1c1c1e"
    readonly property color textPrimary: "#ffffff"
    readonly property color textSecondary: "#8e8e93"
    readonly property color accent: "#3fd0b0"
    readonly property color weekend: "#c46a5f"

    // Typography (empty family = system default)
    readonly property string fontFamily: ""
    readonly property int fontClock: 15
    readonly property int fontBody: 14
    readonly property int fontTitle: 18

    // Island geometry per mode
    readonly property size collapsedSize: Qt.size(116, 34)
    readonly property real collapsedRadius: 17
    readonly property size expandedSize: Qt.size(720, 190)
    readonly property real expandedRadius: 36
    readonly property int topMargin: 6

    // The layer-shell window is fixed and must fit the largest mode; input is masked to the pill.
    readonly property int windowWidth: 760
    readonly property int windowHeight: 420

    // Interaction timing
    readonly property int hoverOpenDelay: 150
    readonly property int hoverCloseDelay: 300

    // Animation
    readonly property int animMorph: 420
    readonly property int easingMorph: Easing.OutBack
    readonly property real easingMorphOvershoot: 1.05
    readonly property int animFade: 180
}
