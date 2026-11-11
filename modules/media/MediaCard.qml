// Compact media card in the Nook tab. Clicking the card (not the buttons) opens the player.
import QtQuick
import QtQuick.Layouts
import qs.components
import qs.services

Item {
    id: root

    readonly property var player: Media.active

    signal openPlayer

    MouseArea {
        anchors.fill: parent
        enabled: root.player !== null
        cursorShape: Qt.PointingHandCursor
        onClicked: root.openPlayer()
    }

    // Nothing playing
    Column {
        anchors.centerIn: parent
        visible: root.player === null
        spacing: 8

        Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            name: "music"
            size: 28
            opacity: 0.3
        }

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Nothing playing"
            color: Theme.textSecondary
        }
    }

    RowLayout {
        anchors.fill: parent
        visible: root.player !== null
        spacing: 16

        Item {
            Layout.preferredWidth: cover.size
            Layout.preferredHeight: cover.size
            Layout.alignment: Qt.AlignVCenter

            Cover {
                id: cover
                size: Math.min(140, root.height)
                source: root.player?.trackArtUrl ?? ""
            }

            AppIcon {
                anchors.right: cover.right
                anchors.bottom: cover.bottom
                anchors.margins: -4
                size: 30
                visible: source !== ""
                source: Media.appIcon(root.player)
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 2

            StyledText {
                Layout.fillWidth: true
                text: root.player?.trackTitle || "Unknown title"
                font.pixelSize: Theme.fontTitle
                font.weight: Font.Bold
            }

            StyledText {
                Layout.fillWidth: true
                visible: text !== ""
                text: root.player?.trackAlbum ?? ""
                color: Theme.textSecondary
                font.weight: Font.Medium
            }

            StyledText {
                Layout.fillWidth: true
                text: root.player?.trackArtist || root.player?.identity || ""
                color: Theme.textSecondary
            }

            Row {
                Layout.topMargin: 8
                spacing: 6

                IconButton {
                    icon: "prev"
                    enabled: root.player?.canGoPrevious ?? false
                    onClicked: root.player.previous()
                }

                IconButton {
                    icon: root.player?.isPlaying ? "pause" : "play"
                    iconSize: 18
                    enabled: root.player?.canTogglePlaying ?? false
                    onClicked: root.player.togglePlaying()
                }

                IconButton {
                    icon: "next"
                    enabled: root.player?.canGoNext ?? false
                    onClicked: root.player.next()
                }
            }
        }
    }
}
