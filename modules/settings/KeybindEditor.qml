pragma ComponentBehavior: Bound
// Keybind rows. Click a key chip, press the new combo (Esc cancels, Backspace clears).
// Changes are saved to the config; on Hyprland binds.lua is regenerated and reloaded.
import QtQuick
import QtQuick.Layouts
import qs.components
import qs.services

ColumnLayout {
    id: root

    // Action being recorded, or "".
    property string capturing: ""

    readonly property var rows: [
        {
            action: "launcher",
            label: "Open launcher"
        },
        {
            action: "toggle",
            label: "Expand / collapse island"
        },
        {
            action: "player",
            label: "Open player"
        },
        {
            action: "calendar",
            label: "Open calendar"
        }
    ]

    function keyName(key: int, text: string): string {
        if (key >= Qt.Key_A && key <= Qt.Key_Z)
            return String.fromCharCode(key);
        if (key >= Qt.Key_0 && key <= Qt.Key_9)
            return String.fromCharCode(key);
        if (key >= Qt.Key_F1 && key <= Qt.Key_F24)
            return "F" + (key - Qt.Key_F1 + 1);
        const named = {
            [Qt.Key_Space]: "Space",
            [Qt.Key_Return]: "Return",
            [Qt.Key_Enter]: "Return",
            [Qt.Key_Tab]: "Tab",
            [Qt.Key_Up]: "Up",
            [Qt.Key_Down]: "Down",
            [Qt.Key_Left]: "Left",
            [Qt.Key_Right]: "Right",
            [Qt.Key_Home]: "Home",
            [Qt.Key_End]: "End",
            [Qt.Key_Print]: "Print",
            [Qt.Key_Comma]: "comma",
            [Qt.Key_Period]: "period",
            [Qt.Key_Slash]: "slash",
            [Qt.Key_Minus]: "minus",
            [Qt.Key_Equal]: "equal",
            [Qt.Key_Semicolon]: "semicolon",
            [Qt.Key_Apostrophe]: "apostrophe",
            [Qt.Key_BracketLeft]: "bracketleft",
            [Qt.Key_BracketRight]: "bracketright",
            [Qt.Key_Backslash]: "backslash",
            [Qt.Key_QuoteLeft]: "grave"
        };
        return named[key] ?? "";
    }

    function handleKey(event: var): void {
        event.accepted = true;
        const key = event.key;
        if (key === Qt.Key_Escape) {
            capturing = "";
            return;
        }
        if ([Qt.Key_Shift, Qt.Key_Control, Qt.Key_Alt, Qt.Key_Meta, Qt.Key_Super_L, Qt.Key_Super_R, Qt.Key_AltGr].includes(key))
            return;
        if (key === Qt.Key_Backspace || key === Qt.Key_Delete) {
            Compositor.setKeybind(capturing, "");
            capturing = "";
            return;
        }
        const name = keyName(key, event.text);
        if (!name)
            return;
        const parts = [];
        if (event.modifiers & Qt.MetaModifier)
            parts.push("SUPER");
        if (event.modifiers & Qt.ControlModifier)
            parts.push("CTRL");
        if (event.modifiers & Qt.AltModifier)
            parts.push("ALT");
        if (event.modifiers & Qt.ShiftModifier)
            parts.push("SHIFT");
        parts.push(name);
        Compositor.setKeybind(capturing, parts.join(" + "));
        capturing = "";
    }

    spacing: 4

    // Receives key presses while recording (the island grabs the keyboard meanwhile).
    Item {
        id: catcher
        focus: root.capturing !== ""
        Keys.onPressed: event => root.handleKey(event)
        onFocusChanged: if (focus)
            forceActiveFocus()
    }

    Repeater {
        model: root.rows

        RowLayout {
            id: row

            required property var modelData
            readonly property string keys: Config.keybinds[modelData.action] ?? ""
            readonly property bool recording: root.capturing === modelData.action

            Layout.fillWidth: true
            Layout.preferredHeight: 40

            StyledText {
                Layout.fillWidth: true
                text: row.modelData.label
            }

            Rectangle {
                Layout.preferredWidth: Math.max(120, chipText.implicitWidth + 24)
                Layout.preferredHeight: 30
                radius: Theme.radiusSmall
                color: row.recording ? Theme.accent : chipArea.containsMouse ? Theme.surfaceHover : Theme.surfaceRaised

                Behavior on color {
                    ColorAnimation {
                        duration: Theme.animFast
                    }
                }

                StyledText {
                    id: chipText
                    anchors.centerIn: parent
                    text: row.recording ? "Press keys…" : (row.keys || "Not set")
                    color: row.recording ? Theme.textOnAccent : row.keys ? Theme.textPrimary : Theme.textSecondary
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }

                MouseArea {
                    id: chipArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.capturing = row.recording ? "" : row.modelData.action
                }
            }
        }
    }
}
