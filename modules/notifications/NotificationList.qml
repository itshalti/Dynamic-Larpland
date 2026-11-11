pragma ComponentBehavior: Bound
// Notification / message history with a clear-all button.
import QtQuick
import QtQuick.Layouts
import qs.components
import qs.services

Item {
    id: root

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        RowLayout {
            Layout.fillWidth: true

            StyledText {
                Layout.fillWidth: true
                text: "Notifications"
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
                font.weight: Font.DemiBold
            }

            StyledText {
                visible: Notifications.history.length > 0
                text: "Clear"
                color: clearArea.containsMouse ? Theme.textPrimary : Theme.textSecondary
                font.pixelSize: Theme.fontSmall

                MouseArea {
                    id: clearArea
                    anchors.fill: parent
                    anchors.margins: -4
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: Notifications.clear()
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                anchors.fill: parent
                clip: true
                spacing: 6
                boundsBehavior: Flickable.StopAtBounds
                model: Notifications.history

                delegate: NotificationItem {
                    required property var modelData

                    width: ListView.view.width
                    entry: modelData
                }
            }

            Column {
                anchors.centerIn: parent
                visible: Notifications.history.length === 0
                spacing: 8

                Icon {
                    anchors.horizontalCenter: parent.horizontalCenter
                    name: "bell"
                    size: 24
                    opacity: 0.3
                }

                StyledText {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "No notifications"
                    color: Theme.textSecondary
                }
            }
        }
    }
}
