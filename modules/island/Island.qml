pragma ComponentBehavior: Bound
// The island window for one screen: a transparent layer-shell surface at the top center
// holding the animated pill. Owns the mode state machine:
//   collapsed | expanded | player | calendar | settings | wallpapers | power
import Quickshell
import Quickshell.Wayland
import QtQuick
import qs.components
import qs.services
import qs.modules.media
import qs.modules.calendar
import qs.modules.settings
import qs.modules.wallpaper
import qs.modules.power

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
        case "wallpapers":
            return Theme.wallpapersSize;
        case "power":
            return Theme.powerSize;
        }
        if (popup)
            return Theme.collapsedNotifSize;
        if (Media.playing)
            return Theme.collapsedMediaSize;
        return Theme.collapsedSize;
    }

    // When the mode last changed: switching pages resizes the pill, which can leave the pointer
    // outside it, so a leave right after a switch waits longer before collapsing.
    property double modeChangedAt: 0

    function scheduleClose(): void {
        const justSwitched = Date.now() - modeChangedAt < Theme.modeSwitchGrace;
        closeTimer.interval = justSwitched ? Theme.modeSwitchCloseDelay : Theme.hoverCloseDelay;
        closeTimer.restart();
    }

    function setMode(next: string): void {
        openTimer.stop();
        closeTimer.stop();
        modeChangedAt = Date.now();
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
    // Don't switch this on expand/collapse: changing it mid-hover made the compositor send a
    // spurious pointer leave, collapsing the island right after it opened.
    WlrLayershell.keyboardFocus: settingsView.capturing ? WlrKeyboardFocus.Exclusive : WlrKeyboardFocus.OnDemand

    // Only the pill receives input; the rest of the window is click-through.
    mask: Region {
        item: shape
    }

    onHoldOpenChanged: {
        if (!holdOpen && expanded && !hover.hovered)
            root.scheduleClose();
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
                        root.scheduleClose();
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
                onOpenWallpapers: root.setMode("wallpapers")
                onOpenPower: root.setMode("power")
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

        ModeView {
            active: root.mode === "wallpapers"
            pageSize: Theme.wallpapersSize

            WallpaperView {
                anchors.fill: parent
                onBack: root.setMode("expanded")
            }
        }

        ModeView {
            active: root.mode === "power"
            pageSize: Theme.powerSize

            PowerView {
                anchors.fill: parent
                onBack: root.setMode("expanded")
            }
        }
    }
}
