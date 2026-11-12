pragma ComponentBehavior: Bound
// Power page (button right of the gear): shut down, reboot, reboot to UEFI, lock, suspend.
// The three that end the session ask for a second click.
import QtQuick
import QtQuick.Layouts
import qs.components
import qs.services

Item {
    id: root

    signal back

    readonly property var actions: [
        {
            action: "poweroff",
            icon: "power",
            label: "Shut down",
            confirm: true
        },
        {
            action: "reboot",
            icon: "reboot",
            label: "Reboot",
            confirm: true
        },
        {
            action: "firmware",
            icon: "firmware",
            label: "UEFI",
            confirm: true
        },
        {
            action: "lock",
            icon: "lock",
            label: "Lock",
            confirm: false
        },
        {
            action: "suspend",
            icon: "sleep",
            label: "Sleep",
            confirm: false
        }
    ]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 22
        anchors.topMargin: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true

            IconButton {
                icon: "chevron-left"
                onClicked: root.back()
            }

            StyledText {
                Layout.fillWidth: true
                text: "Power"
                font.pixelSize: Theme.fontLarge
                font.weight: Font.Bold
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 12

            Repeater {
                model: root.actions

                PowerButton {
                    required property var modelData

                    icon: modelData.icon
                    label: modelData.label
                    confirm: modelData.confirm
                    onTriggered: {
                        root.back();
                        Power.run(modelData.action);
                    }
                }
            }
        }
    }
}
