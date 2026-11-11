pragma ComponentBehavior: Bound
// Expanded island (NotchNook-like): Nook / Tray tabs and a settings gear on top;
// Nook = media card | week strip | notifications, Tray = system tray.
import QtQuick
import QtQuick.Layouts
import qs.components
import qs.modules.media
import qs.modules.calendar
import qs.modules.notifications
import qs.modules.tray

Item {
    id: root

    property string tab: "nook"
    readonly property bool holdOpen: tray.menuOpen

    signal openPlayer
    signal openCalendar
    signal openSettings


    RowLayout {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 16
        anchors.leftMargin: 26
        spacing: 6

        TabButton {
            icon: "music"
            label: "Nook"
            selected: root.tab === "nook"
            onClicked: root.tab = "nook"
        }

        TabButton {
            icon: "tray"
            label: "Tray"
            selected: root.tab === "tray"
            onClicked: root.tab = "tray"
        }

        Item {
            Layout.fillWidth: true
        }

        IconButton {
            icon: "gear"
            iconSize: 18
            onClicked: root.openSettings()
        }
    }

    Item {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 26
        anchors.topMargin: 12
        anchors.bottomMargin: 20

        RowLayout {
            anchors.fill: parent
            spacing: 18
            visible: root.tab === "nook"

            MediaCard {
                Layout.preferredWidth: 320
                Layout.fillHeight: true
                onOpenPlayer: root.openPlayer()
            }

            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                Layout.topMargin: 6
                Layout.bottomMargin: 6
                color: Theme.divider
            }

            WeekStrip {
                clip: true
                Layout.fillWidth: true
                Layout.fillHeight: true
                onOpenCalendar: root.openCalendar()
            }

            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                Layout.topMargin: 6
                Layout.bottomMargin: 6
                color: Theme.divider
            }

            NotificationList {
                Layout.preferredWidth: 260
                Layout.fillHeight: true
            }
        }

        TrayView {
            id: tray
            anchors.fill: parent
            visible: root.tab === "tray"
        }
    }
}
