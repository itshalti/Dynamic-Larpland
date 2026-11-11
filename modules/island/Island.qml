pragma ComponentBehavior: Bound
// The island window for one screen: a transparent layer-shell surface at the top center
// holding the animated pill. Owns the mode state machine:
//   collapsed | expanded | player | calendar | settings
import Quickshell
import Quickshell.Wayland
import QtQuick
import qs.components
import qs.services
import qs.modules.media
import qs.modules.calendar
import qs.modules.settings

PanelWindow {
    id: root

    required property ShellScreen modelData

    property string mode: "collapsed"
    readonly property bool expanded: mode !== "collapsed"
    // Notification entry shown as a live activity while collapsed.
    property var popup: null

    // Actions from keybinds/IPC go to the island on the focused screen only.
    readonly property bool isTarget: !Compositor.focusedScreen || Compositor.focusedScreen === modelData.name
    // Don't collapse when the pointer leaves (settings page, open tray menu).
    readonly property bool holdOpen: mode === "settings" || expandedView.holdOpen

    readonly property size targetSize: {
        switch (mode) {
        case "expanded":
            return Theme.expandedSize;
        case "player":
            return Theme.playerSize;
        case "calendar":
            return Theme.calendarSize;
        case "settings":
            return Theme.settingsSize;
        }
        if (popup)
            return Theme.collapsedNotifSize;
        if (Media.playing)
            return Theme.collapsedMediaSize;
        return Theme.collapsedSize;
    }

    function setMode(next: string): void {
        openTimer.stop();
        closeTimer.stop();
        if (next !== "collapsed")
            popup = null;
        mode = next;
        if (next !== "collapsed")
            keys.forceActiveFocus();
    }

    function collapse(): void {
        setMode("collapsed");
    }

    function toggle(): void {
        setMode(expanded ? "collapsed" : "expanded");
    }

    screen: modelData
    anchors.top: true
    margins.top: Theme.topMargin
    implicitWidth: Theme.windowWidth
    implicitHeight: Theme.windowHeight
    color: "transparent"
    exclusionMode: ExclusionMode.Ignore
    WlrLayershell.layer: WlrLayer.Overlay
    WlrLayershell.namespace: "dynamic-larpland"
    // Grab the keyboard only while recording a keybind; otherwise take focus on click (for Esc).
    WlrLayershell.keyboardFocus: settingsView.capturing ? WlrKeyboardFocus.Exclusive : expanded ? WlrKeyboardFocus.OnDemand : WlrKeyboardFocus.None

    // Only the pill receives input; the rest of the window is click-through.
    mask: Region {
        item: shape
    }

    onHoldOpenChanged: {
        if (!holdOpen && expanded && !hover.hovered)
            closeTimer.restart();
    }

    Connections {
        target: Ipc

        function onToggleRequested(): void {
            if (root.isTarget)
                root.toggle();
        }

        function onCollapseRequested(): void {
            root.collapse();
        }

        function onModeRequested(mode: string): void {
            if (root.isTarget)
                root.setMode(root.mode === mode ? "collapsed" : mode);
        }
    }

    Connections {
        target: Notifications

        function onPosted(entry: var): void {
            if (root.expanded)
                return;
            root.popup = entry;
            popupTimer.restart();
        }
    }

    Timer {
        id: popupTimer
        interval: Config.notifications.popupTimeoutMs
        onTriggered: root.popup = null
    }

    Timer {
        id: openTimer
        interval: Config.island.hoverDelayMs
        onTriggered: root.setMode("expanded")
    }

    Timer {
        id: closeTimer
        interval: Theme.hoverCloseDelay
        onTriggered: {
            if (!root.holdOpen)
                root.collapse();
        }
    }

    IslandShape {
        id: shape

        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        targetSize: root.targetSize
        targetRadius: root.expanded ? Theme.expandedRadius : root.targetSize.height / 2

        Item {
            id: keys
            anchors.fill: parent
            focus: true
            Keys.onEscapePressed: root.collapse()
        }

        HoverHandler {
            id: hover

            onHoveredChanged: {
                if (hovered) {
                    closeTimer.stop();
                    if (!root.expanded && Config.island.hoverExpand)
                        openTimer.restart();
                } else {
                    openTimer.stop();
                    if (root.expanded && !root.holdOpen)
                        closeTimer.restart();
                }
            }
        }

        TapHandler {
            enabled: !root.expanded
            onTapped: root.setMode("expanded")
        }

        ModeView {
            active: root.mode === "collapsed"
            pageSize: root.targetSize

            CollapsedView {
                anchors.fill: parent
                popup: root.popup
            }
        }

        ModeView {
            active: root.mode === "expanded"
            pageSize: Theme.expandedSize

            ExpandedView {
                id: expandedView
                anchors.fill: parent
                onOpenPlayer: root.setMode("player")
                onOpenCalendar: root.setMode("calendar")
                onOpenSettings: root.setMode("settings")
            }
        }

        ModeView {
            active: root.mode === "player"
            pageSize: Theme.playerSize

            PlayerView {
                anchors.fill: parent
                onBack: root.setMode("expanded")
            }
        }

        ModeView {
            active: root.mode === "calendar"
            pageSize: Theme.calendarSize

            MonthCalendar {
                anchors.fill: parent
            }
        }

        ModeView {
            active: root.mode === "settings"
            pageSize: Theme.settingsSize

            SettingsView {
                id: settingsView
                anchors.fill: parent
                onBack: root.setMode("expanded")
            }
        }
    }
}
