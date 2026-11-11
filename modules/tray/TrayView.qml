pragma ComponentBehavior: Bound
// System tray (StatusNotifierItem) as a compact icon row for the island header.
// Left click activates (or opens the menu for menu-only items), right click opens the menu,
// middle click is the secondary action, wheel scrolls.
import QtQuick
import Quickshell
import Quickshell.Services.SystemTray
import Quickshell.Widgets
import qs.components

Row {
    id: root

    // While a tray menu is open the island must not collapse under it.
    readonly property bool menuOpen: menu.visible

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

    spacing: 4

    QsMenuAnchor {
        id: menu
        anchor.edges: Edges.Bottom
        anchor.gravity: Edges.Bottom
    }

    Repeater {
        model: SystemTray.items

        Rectangle {
            id: tile

            required property SystemTrayItem modelData

            width: 32
            height: 32
            radius: height / 2
            color: area.containsMouse ? Theme.surfaceHover : "transparent"

            Behavior on color {
                ColorAnimation {
                    duration: Theme.animFast
                }
            }

            IconImage {
                anchors.centerIn: parent
                implicitSize: 18
                source: root.iconSource(tile.modelData.icon)
            }

            MouseArea {
                id: area
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                cursorShape: Qt.PointingHandCursor
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
