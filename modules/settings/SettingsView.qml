// Settings page opened from the gear: keybinds and island behaviour.
import QtQuick
import QtQuick.Layouts
import qs.components
import qs.services

Item {
    id: root

    readonly property bool capturing: keybinds.capturing !== ""

    signal back


    // Leaving the page stops any recording.
    onVisibleChanged: if (!visible)
        keybinds.capturing = ""

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 22
        anchors.topMargin: 16
        spacing: 6

        RowLayout {
            Layout.fillWidth: true

            IconButton {
                icon: "chevron-left"
                onClicked: root.back()
            }

            StyledText {
                Layout.fillWidth: true
                text: "Settings"
                font.pixelSize: Theme.fontLarge
                font.weight: Font.Bold
            }
        }

        StyledText {
            Layout.topMargin: 6
            text: "KEYBINDS"
            color: Theme.textSecondary
            font.pixelSize: 11
            font.weight: Font.DemiBold
            font.letterSpacing: 1
        }

        KeybindEditor {
            id: keybinds
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 40

            StyledText {
                Layout.fillWidth: true
                text: "Expand on hover"
            }

            Toggle {
                checked: Config.island.hoverExpand
                onToggled: Config.island.hoverExpand = !Config.island.hoverExpand
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            spacing: 12

            StyledText {
                text: "Wallpaper folder"
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                radius: Theme.radiusSmall
                color: Theme.surfaceRaised
                border.width: dirInput.activeFocus ? 1 : 0
                border.color: Theme.accent

                TextInput {
                    id: dirInput
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    verticalAlignment: TextInput.AlignVCenter
                    clip: true
                    text: Config.wallpaper.dir
                    color: Theme.textPrimary
                    selectionColor: Theme.accent
                    selectedTextColor: Theme.textOnAccent
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontBody
                    // Saved on Enter or when the field loses focus.
                    onEditingFinished: if (text.trim())
                        Config.wallpaper.dir = text.trim()
                    Keys.onEscapePressed: {
                        text = Config.wallpaper.dir;
                        focus = false;
                    }
                }
            }
        }

        StyledText {
            Layout.fillWidth: true
            Layout.fillHeight: true
            verticalAlignment: Text.AlignBottom
            wrapMode: Text.Wrap
            elide: Text.ElideNone
            color: Theme.textTertiary
            font.pixelSize: 11
            text: Compositor.isHyprland ? `Saved to ${Compositor.bindsPathDisplay} — load it from your Hyprland config with: ${Compositor.includeLine}` : `Bind keys in your compositor to: ${Compositor.ipcCommand("<action>")}`
        }
    }
}
