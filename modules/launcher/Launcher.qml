pragma ComponentBehavior: Bound
// App launcher (default Super+Space). A full-screen transparent overlay on the focused screen;
// clicking outside the card, Esc or launching an app closes it.
import Quickshell
import Quickshell.Wayland
import QtQuick
import QtQuick.Layouts
import qs.components
import qs.services

PanelWindow {
    id: root

    property bool open: false
    readonly property var results: Apps.query(input.text)

    function show(): void {
        screen = Quickshell.screens.find(s => s.name === Compositor.focusedScreen) ?? Quickshell.screens[0];
        input.text = "";
        list.currentIndex = 0;
        open = true;
        input.forceActiveFocus();
    }

    function hide(): void {
        open = false;
    }

    function launch(entry: var): void {
        if (!entry)
            return;
        Apps.launch(entry);
        hide();
    }

    visible: open || card.opacity > 0
    anchors {
        top: true
        bottom: true
        left: true
        right: true
    }
    color: "transparent"
    exclusionMode: ExclusionMode.Ignore
    WlrLayershell.layer: WlrLayer.Overlay
    WlrLayershell.namespace: "dynamic-larpland-launcher"
    WlrLayershell.keyboardFocus: open ? WlrKeyboardFocus.Exclusive : WlrKeyboardFocus.None

    Connections {
        target: Ipc

        function onLauncherRequested(): void {
            root.open ? root.hide() : root.show();
        }
    }

    // Click outside the card closes.
    MouseArea {
        anchors.fill: parent
        enabled: root.open
        onClicked: root.hide()
    }

    Rectangle {
        id: card

        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height * 0.2
        width: Theme.launcherWidth
        height: content.implicitHeight + 24
        radius: 30
        color: Theme.surface
        border.width: 1
        border.color: Theme.divider
        opacity: root.open ? 1 : 0
        scale: root.open ? 1 : 0.95
        transformOrigin: Item.Top

        Behavior on opacity {
            NumberAnimation {
                duration: Theme.animFade
            }
        }

        Behavior on scale {
            NumberAnimation {
                duration: Theme.animMorph * 0.7
                easing.type: Theme.easingMorph
            }
        }

        Behavior on height {
            NumberAnimation {
                duration: Theme.animFast
                easing.type: Easing.OutQuad
            }
        }

        // Swallow clicks so they don't reach the close area.
        MouseArea {
            anchors.fill: parent
        }

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 12
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                Layout.leftMargin: 14
                Layout.rightMargin: 14
                spacing: 14

                Icon {
                    name: "search"
                    size: 20
                    opacity: 0.55
                }

                TextInput {
                    id: input

                    Layout.fillWidth: true
                    focus: true
                    color: Theme.textPrimary
                    selectionColor: Theme.accent
                    selectedTextColor: Theme.textOnAccent
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontTitle
                    clip: true
                    onTextChanged: list.currentIndex = 0

                    Keys.onEscapePressed: root.hide()
                    Keys.onReturnPressed: root.launch(root.results[list.currentIndex])
                    Keys.onEnterPressed: root.launch(root.results[list.currentIndex])
                    Keys.onUpPressed: list.decrementCurrentIndex()
                    Keys.onDownPressed: list.incrementCurrentIndex()
                    Keys.onTabPressed: list.incrementCurrentIndex()
                    Keys.onBacktabPressed: list.decrementCurrentIndex()

                    StyledText {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: input.text === ""
                        text: "Search apps"
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontTitle
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                visible: root.results.length > 0
                color: Theme.divider
            }

            ListView {
                id: list

                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(root.results.length, Theme.launcherMaxRows) * (Theme.launcherRowHeight + spacing)
                clip: true
                spacing: 4
                model: root.results
                boundsBehavior: Flickable.StopAtBounds
                highlightMoveDuration: 0
                keyNavigationWraps: true

                delegate: LauncherItem {
                    required property var modelData
                    required property int index

                    width: ListView.view.width
                    entry: modelData
                    selected: ListView.isCurrentItem
                    onHovered: list.currentIndex = index
                    onActivated: root.launch(modelData)
                }
            }
        }
    }
}
