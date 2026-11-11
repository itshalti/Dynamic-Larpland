// Plain gradient wallpaper for the nested dev session (it has no wallpaper daemon).
// Started by dev/hyprland-nested.lua as a separate Quickshell config.
import Quickshell
import Quickshell.Wayland
import QtQuick

ShellRoot {
    Variants {
        model: Quickshell.screens

        PanelWindow {
            required property ShellScreen modelData

            screen: modelData
            anchors {
                top: true
                bottom: true
                left: true
                right: true
            }
            exclusionMode: ExclusionMode.Ignore
            WlrLayershell.layer: WlrLayer.Background
            WlrLayershell.namespace: "dynamic-larpland-wallpaper"

            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop {
                        position: 0
                        color: "#0f3d3e"
                    }
                    GradientStop {
                        position: 0.55
                        color: "#1d2b53"
                    }
                    GradientStop {
                        position: 1
                        color: "#4a1d5c"
                    }
                }
            }
        }
    }
}
