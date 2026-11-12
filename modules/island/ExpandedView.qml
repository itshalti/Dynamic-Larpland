pragma ComponentBehavior: Bound
// Expanded island (NotchNook-like). Header: Nook label, system tray icons, then screen record,
// wallpapers, the settings gear and power.
// Body: media card | week strip | notifications.
import QtQuick
import QtQuick.Layouts
import qs.components
import qs.services
import qs.modules.media
import qs.modules.calendar
import qs.modules.notifications
import qs.modules.tray

Item {
    id: root

    readonly property bool holdOpen: tray.menuOpen

    signal openPlayer
    signal openCalendar
    signal openSettings
    signal openWallpapers
    signal openPower

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
            selected: true
        }

        TrayView {
            id: tray
            Layout.leftMargin: 8
        }

        Item {
            Layout.fillWidth: true
        }

        IconButton {
            icon: Recorder.recording ? "stop" : "record"
            iconSize: 18
            baseColor: Recorder.recording ? Theme.recording : "transparent"
            onClicked: Recorder.toggle()
        }

        IconButton {
            icon: "image"
            iconSize: 18
            onClicked: root.openWallpapers()
        }

        IconButton {
            icon: "gear"
            iconSize: 18
            onClicked: root.openSettings()
        }

        IconButton {
            icon: "power"
            iconSize: 18
            onClicked: root.openPower()
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
    }
}
