pragma Singleton
// Design tokens. Modules must take every color, size, radius and timing from here.
import Quickshell
import QtQuick

Singleton {
    // Colors
    readonly property color surface: "#000000"
    readonly property color surfaceRaised: "#1c1c1e"
    readonly property color surfaceHover: "#2a2a2c"
    readonly property color divider: "#232325"
    readonly property color textPrimary: "#ffffff"
    readonly property color textSecondary: "#8e8e93"
    readonly property color textTertiary: "#4a4a4e"
    readonly property color accent: "#3fd0b0"
    readonly property color textOnAccent: "#000000"
    readonly property color weekend: "#c46a5f"
    readonly property color track: "#3a3a3c"

    // Typography ("sans-serif" resolves through fontconfig)
    readonly property string fontFamily: "sans-serif"
    readonly property int fontSmall: 12
    readonly property int fontBody: 14
    readonly property int fontLarge: 17
    readonly property int fontTitle: 20
    readonly property int fontHuge: 30

    // Radii
    readonly property real radiusSmall: 8
    readonly property real radiusMedium: 14
    readonly property real radiusLarge: 20

    // Island geometry per mode
    readonly property size collapsedSize: Qt.size(116, 34)
    readonly property size collapsedMediaSize: Qt.size(210, 34)
    readonly property size collapsedNotifSize: Qt.size(360, 34)
    readonly property size expandedSize: Qt.size(1040, 232)
    readonly property size playerSize: Qt.size(470, 196)
    readonly property size calendarSize: Qt.size(440, 376)
    readonly property size settingsSize: Qt.size(560, 380)
    readonly property real expandedRadius: 36
    readonly property int topMargin: 6

    // The layer-shell window is fixed and must fit the largest mode; input is masked to the pill.
    readonly property int windowWidth: 1080
    readonly property int windowHeight: 420

    // Launcher
    readonly property int launcherWidth: 680
    readonly property int launcherRowHeight: 60
    readonly property int launcherMaxRows: 7

    // Interaction timing
    readonly property int hoverCloseDelay: 300

    // Animation
    readonly property int animMorph: 420
    readonly property int easingMorph: Easing.OutBack
    readonly property real easingMorphOvershoot: 1.05
    readonly property int animFade: 160
    readonly property int animFast: 110
}
