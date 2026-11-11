// Full player: cover, title, seekable progress, transport controls, player switching.
import QtQuick
import QtQuick.Layouts
import qs.components
import qs.services

Item {
    id: root

    readonly property var player: Media.active
    readonly property real length: player?.length ?? 0
    // -1 while not dragging; otherwise the ratio under the pointer.
    property real dragRatio: -1
    readonly property real ratio: dragRatio >= 0 ? dragRatio : (length > 0 ? Math.min(1, (player?.position ?? 0) / length) : 0)

    signal back

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 22
        anchors.topMargin: 20
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            Cover {
                size: 58
                source: root.player?.trackArtUrl ?? ""

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.back()
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                StyledText {
                    Layout.fillWidth: true
                    text: root.player?.trackTitle || "Nothing playing"
                    font.pixelSize: Theme.fontLarge
                    font.weight: Font.Bold
                }

                StyledText {
                    Layout.fillWidth: true
                    text: root.player?.trackArtist || root.player?.identity || ""
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontLarge
                }
            }

            Visualizer {
                maxHeight: 26
                playing: Media.playing
                bars: 6
                barWidth: 3.5
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            StyledText {
                text: Media.formatTime(root.ratio * root.length)
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
                font.features: ({
                        "tnum": 1
                    })
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 16

                Rectangle {
                    id: track
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width
                    height: seek.containsMouse || seek.pressed ? 8 : 6
                    radius: height / 2
                    color: Theme.track

                    Behavior on height {
                        NumberAnimation {
                            duration: Theme.animFast
                        }
                    }

                    Rectangle {
                        width: track.width * root.ratio
                        height: parent.height
                        radius: parent.radius
                        color: seek.pressed ? Theme.textPrimary : Theme.textSecondary
                    }
                }

                MouseArea {
                    id: seek

                    function ratioAt(x: real): real {
                        return Math.max(0, Math.min(1, x / width));
                    }

                    anchors.fill: parent
                    enabled: root.player?.canSeek ?? false
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onPressed: mouse => root.dragRatio = ratioAt(mouse.x)
                    onPositionChanged: mouse => {
                        if (pressed)
                            root.dragRatio = ratioAt(mouse.x);
                    }
                    onReleased: {
                        root.player.position = root.dragRatio * root.length;
                        root.dragRatio = -1;
                    }
                }
            }

            StyledText {
                text: "-" + Media.formatTime((1 - root.ratio) * root.length)
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Row {
                anchors.centerIn: parent
                spacing: 28

                IconButton {
                    anchors.verticalCenter: parent.verticalCenter
                    size: 46
                    icon: "rewind"
                    iconSize: 28
                    enabled: root.player?.canGoPrevious ?? false
                    onClicked: root.player.previous()
                }

                IconButton {
                    anchors.verticalCenter: parent.verticalCenter
                    size: 52
                    icon: root.player?.isPlaying ? "pause" : "play"
                    iconSize: 32
                    enabled: root.player?.canTogglePlaying ?? false
                    onClicked: root.player.togglePlaying()
                }

                IconButton {
                    anchors.verticalCenter: parent.verticalCenter
                    size: 46
                    icon: "forward"
                    iconSize: 28
                    enabled: root.player?.canGoNext ?? false
                    onClicked: root.player.next()
                }
            }

            IconButton {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                visible: Media.players.length > 1
                icon: "swap"
                iconSize: 18
                onClicked: Media.cyclePlayer()
            }
        }
    }
}
