pragma ComponentBehavior: Bound
// System tray (StatusNotifierItem). Left click activates (or opens the menu for menu-only
// items), right click opens the menu, middle click is the secondary action.
import QtQuick
import Quickshell
import Quickshell.Services.SystemTray
import Quickshell.Widgets
import qs.components

Item {
    id: root

    // While a tray menu is open the island must not collapse under it.
    readonly property bool menuOpen: menu.visible
    property string hoveredTitle: ""

    // Some apps (Electron) report "name?path=/dir"; turn that into a file URL.
    function iconSource(icon: string): string {
        if (!icon.includes("?path="))
            return icon;
        const [name, path] = icon.split("?path=");
        return `file://${path}/${name.slice(name.lastIndexOf("/") + 1)}`;
    }

    function openMenu(item: SystemTrayItem, anchorItem: Item): void {
        menu.menu = item.menu;
        menu.anchor.item = anchorItem;
        menu.open();
    }

    QsMenuAnchor {
        id: menu
        anchor.edges: Edges.Bottom
        anchor.gravity: Edges.Bottom
    }

    Column {
        anchors.centerIn: parent
        spacing: 10

        Flow {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(implicitWidth, root.width)
            spacing: 8

            Repeater {
                model: SystemTray.items

                Rectangle {
                    id: tile

                    required property SystemTrayItem modelData

                    width: 46
                    height: 46
                    radius: Theme.radiusMedium
                    color: area.containsMouse ? Theme.surfaceHover : Theme.surfaceRaised

                    Behavior on color {
                        ColorAnimation {
                            duration: Theme.animFast
                        }
                    }

                    IconImage {
                        anchors.centerIn: parent
                        implicitSize: 24
                        source: root.iconSource(tile.modelData.icon)
                    }

                    MouseArea {
                        id: area
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                        cursorShape: Qt.PointingHandCursor
                        onContainsMouseChanged: root.hoveredTitle = containsMouse ? (tile.modelData.tooltipTitle || tile.modelData.title || tile.modelData.id) : ""
                        onClicked: mouse => {
                            const item = tile.modelData;
                            if (mouse.button === Qt.MiddleButton)
                                item.secondaryActivate();
                            else if (mouse.button === Qt.RightButton || item.onlyMenu) {
                                if (item.hasMenu)
                                    root.openMenu(item, tile);
                            } else
                                item.activate();
                        }
                        onWheel: wheel => tile.modelData.scroll(wheel.angleDelta.y / 120, false)
                    }
                }
            }
        }

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            text: SystemTray.items.values.length === 0 ? "No tray icons" : (root.hoveredTitle || " ")
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSmall
        }
    }
}
