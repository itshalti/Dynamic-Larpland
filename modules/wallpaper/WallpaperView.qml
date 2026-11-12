pragma ComponentBehavior: Bound
// Wallpaper picker: the images of the wallpaper folder in a horizontal strip (wheel scrolls).
// Picking one sets it and recolors the system through matugen (services/Wallpaper).
import QtQuick
import QtQuick.Layouts
import qs.components
import qs.services

Item {
    id: root

    signal back

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 22
        anchors.topMargin: 16
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            IconButton {
                icon: "chevron-left"
                onClicked: root.back()
            }

            StyledText {
                text: "Wallpapers"
                font.pixelSize: Theme.fontLarge
                font.weight: Font.Bold
            }

            StyledText {
                Layout.fillWidth: true
                text: Config.wallpaper.dir
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSmall
            }
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: ListView.Horizontal
            spacing: 14
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: Wallpaper.images

            delegate: WallpaperItem {
                required property string filePath
                required property string fileName

                anchors.verticalCenter: parent?.verticalCenter
                path: filePath
                name: fileName
                current: filePath === Wallpaper.current
                onPicked: Wallpaper.set(filePath)
            }

            // Mouse wheel (either axis) flicks the strip sideways; down/right = forward.
            WheelHandler {
                target: null
                onWheel: event => list.flick((event.angleDelta.y || event.angleDelta.x) * 8, 0)
            }
        }
    }

    StyledText {
        anchors.centerIn: parent
        visible: list.count === 0
        text: `No images in ${Config.wallpaper.dir}`
        color: Theme.textSecondary
    }
}
