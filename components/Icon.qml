// A bundled white SVG icon from assets/icons. Dim it with `opacity`.
import QtQuick

Image {
    id: root

    property string name
    property real size: 16

    width: size
    height: size
    sourceSize: Qt.size(size * 2, size * 2)
    source: name ? Qt.resolvedUrl("../assets/icons/" + name + ".svg") : ""
    fillMode: Image.PreserveAspectFit
    smooth: true
}
