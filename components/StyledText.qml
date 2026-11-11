// Text with the theme's font and color defaults.
import QtQuick
import qs.components

Text {
    color: Theme.textPrimary
    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontBody
    elide: Text.ElideRight
    verticalAlignment: Text.AlignVCenter
}
