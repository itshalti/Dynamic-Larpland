pragma ComponentBehavior: Bound
// The island window for one screen: a transparent layer-shell surface at the top center
// holding the animated pill. Owns the mode state machine.
import Quickshell
import Quickshell.Wayland
import QtQuick
import qs.components
import qs.services

PanelWindow {
    id: root

    required property ShellScreen modelData

    // collapsed | expanded  (player | calendar | settings come later)
    property string mode: "collapsed"
    readonly property bool expanded: mode !== "collapsed"

    function expand(): void {
        closeTimer.stop();
        mode = "expanded";
    }

    function collapse(): void {
        openTimer.stop();
        mode = "collapsed";
    }

    function toggle(): void {
        expanded ? collapse() : expand();
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

    // Only the pill receives input; the rest of the window is click-through.
    mask: Region {
        item: shape
    }

    Connections {
        target: Ipc

        function onToggleRequested(): void {
            root.toggle();
        }

        function onExpandRequested(): void {
            root.expand();
        }

        function onCollapseRequested(): void {
            root.collapse();
        }
    }

    Timer {
        id: openTimer
        interval: Theme.hoverOpenDelay
        onTriggered: root.expand()
    }

    Timer {
        id: closeTimer
        interval: Theme.hoverCloseDelay
        onTriggered: root.collapse()
    }

    IslandShape {
        id: shape

        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        targetSize: root.expanded ? Theme.expandedSize : Theme.collapsedSize
        targetRadius: root.expanded ? Theme.expandedRadius : Theme.collapsedRadius

        HoverHandler {
            onHoveredChanged: {
                if (hovered) {
                    closeTimer.stop();
                    if (!root.expanded)
                        openTimer.restart();
                } else {
                    openTimer.stop();
                    if (root.expanded)
                        closeTimer.restart();
                }
            }
        }

        TapHandler {
            onTapped: root.toggle()
        }

        CollapsedView {
            anchors.fill: parent
            opacity: root.expanded ? 0 : 1
            visible: opacity > 0

            Behavior on opacity {
                NumberAnimation {
                    duration: Theme.animFade
                }
            }
        }

        ExpandedView {
            anchors.fill: parent
            opacity: root.expanded ? 1 : 0
            visible: opacity > 0

            Behavior on opacity {
                NumberAnimation {
                    duration: Theme.animFade
                }
            }
        }
    }
}
