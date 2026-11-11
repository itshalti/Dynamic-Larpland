// Collapsed content: the clock, or a compact live activity —
// a fresh notification (icon + text) or the playing track (cover, clock, bars).
import QtQuick
import QtQuick.Layouts
import qs.components
import qs.services
import qs.modules.media

Item {
    id: root

    // Notification history entry shown as a live activity, or null.
    property var popup: null
    readonly property bool showMedia: popup === null && Media.playing

    // Plain clock
    StyledText {
        anchors.centerIn: parent
        visible: root.popup === null && !root.showMedia
        text: Time.time
        font.pixelSize: 15
        font.weight: Font.DemiBold
    }

    // Playing track
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 7
        anchors.rightMargin: 14
        visible: root.showMedia
        spacing: 8

        Cover {
            size: 22
            source: Media.active?.trackArtUrl ?? ""
        }

        StyledText {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: Time.time
            font.pixelSize: 15
            font.weight: Font.DemiBold
        }

        Visualizer {
            playing: Media.playing
            color: Theme.accent
            bars: 4
            barWidth: 2.5
            maxHeight: 14
        }
    }

    // New notification
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 7
        anchors.rightMargin: 16
        visible: root.popup !== null
        spacing: 9

        AppIcon {
            size: 22
            radius: 11
            source: root.popup?.icon ?? ""
        }

        StyledText {
            Layout.fillWidth: true
            text: root.popup ? (root.popup.summary || root.popup.appName) : ""
            textFormat: Text.PlainText
            font.pixelSize: 13
            font.weight: Font.DemiBold
        }

        StyledText {
            Layout.maximumWidth: 110
            text: root.popup?.appName ?? ""
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSmall
        }
    }
}
