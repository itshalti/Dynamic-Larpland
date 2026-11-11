// One history entry: app icon (or the sender's image), summary, age and body.
// Click runs the app's default action; the × button removes it.
import QtQuick
import QtQuick.Layouts
import qs.components
import qs.services

Rectangle {
    id: root

    required property var entry

    implicitHeight: 54
    radius: Theme.radiusMedium
    color: hover.hovered ? Theme.surfaceHover : Theme.surfaceRaised

    Behavior on color {
        ColorAnimation {
            duration: Theme.animFast
        }
    }

    HoverHandler {
        id: hover
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: Notifications.activate(root.entry.key)
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 8
        spacing: 10

        Item {
            Layout.preferredWidth: 30
            Layout.preferredHeight: 30

            // The sender's image (avatar) only exists while the notification is alive.
            AppIcon {
                id: image
                anchors.fill: parent
                size: 30
                radius: 15
                source: root.entry.image ?? ""
                visible: ready
            }

            AppIcon {
                anchors.fill: parent
                size: 30
                visible: !image.ready
                source: root.entry.icon ?? ""
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                StyledText {
                    Layout.fillWidth: true
                    text: root.entry.summary || root.entry.appName
                    textFormat: Text.PlainText
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }

                StyledText {
                    text: Notifications.ago(root.entry.time, Time.now)
                    color: Theme.textSecondary
                    font.pixelSize: 11
                }
            }

            StyledText {
                Layout.fillWidth: true
                visible: text !== ""
                text: (root.entry.body ?? "").replace(/<[^>]*>/g, "").replace(/\s+/g, " ")
                textFormat: Text.PlainText
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
            }
        }

        IconButton {
            size: 22
            icon: "close"
            iconSize: 12
            opacity: hover.hovered ? 1 : 0
            onClicked: Notifications.remove(root.entry.key)
        }
    }
}
