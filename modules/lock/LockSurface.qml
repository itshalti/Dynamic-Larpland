// Lock screen content: the current wallpaper, a big clock in the middle and, once the user starts
// typing, a password field at the bottom that shows one * per character. Enter unlocks, Esc clears.
import QtQuick
import qs.components
import qs.services

Item {
    id: root

    focus: true
    Component.onCompleted: forceActiveFocus()

    Keys.onPressed: event => {
        event.accepted = true;
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
            Lock.submit();
        else if (event.key === Qt.Key_Escape)
            Lock.input = "";
        else if (event.key === Qt.Key_Backspace)
            Lock.input = Lock.input.slice(0, -1);
        else if (event.text && event.text.charCodeAt(0) >= 32 && !Lock.checking)
            Lock.input += event.text;
    }

    Image {
        anchors.fill: parent
        source: Wallpaper.current ? "file://" + Wallpaper.current : ""
        sourceSize: Qt.size(root.width, root.height)
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
    }

    // Keeps the white text readable on bright wallpapers.
    Rectangle {
        anchors.fill: parent
        color: Theme.lockDim
    }

    Column {
        anchors.centerIn: parent
        spacing: 4

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Time.time
            font.pixelSize: Theme.lockClockSize
            font.weight: Font.Bold
        }

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Qt.formatDate(Time.now, "dddd, d MMMM")
            font.pixelSize: Theme.fontTitle
            opacity: 0.85
        }
    }

    Column {
        id: bottom

        readonly property bool shown: Lock.input !== "" || Lock.checking || Lock.error !== ""

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: shown ? Theme.lockFieldMargin : Theme.lockFieldMargin - 30
        spacing: 10
        opacity: shown ? 1 : 0

        Behavior on opacity {
            NumberAnimation {
                duration: Theme.animFade
            }
        }

        Behavior on anchors.bottomMargin {
            NumberAnimation {
                duration: Theme.animMorph
                easing.type: Theme.easingMorph
            }
        }

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Lock.error
            color: Theme.recording
            font.pixelSize: Theme.fontBody
            opacity: Lock.error ? 1 : 0
        }

        Rectangle {
            id: field

            width: Theme.lockFieldSize.width
            height: Theme.lockFieldSize.height
            radius: height / 2
            color: Theme.surface
            border.width: 1
            border.color: Lock.error ? Theme.recording : Theme.divider

            Icon {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                name: "lock"
                size: 18
                opacity: 0.6
            }

            StyledText {
                anchors.centerIn: parent
                width: parent.width - 100
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideLeft
                text: Lock.checking ? "Checking…" : "*".repeat(Lock.input.length)
                color: Lock.checking ? Theme.textSecondary : Theme.textPrimary
                font.pixelSize: Lock.checking ? Theme.fontBody : Theme.fontTitle
                font.letterSpacing: Lock.checking ? 0 : 4
            }

            // Shake on a wrong password.
            transform: Translate {
                id: shakeOffset
            }

            SequentialAnimation {
                id: shake
                loops: 2
                NumberAnimation {
                    target: shakeOffset
                    property: "x"
                    to: -12
                    duration: Theme.animFast / 2
                }
                NumberAnimation {
                    target: shakeOffset
                    property: "x"
                    to: 12
                    duration: Theme.animFast
                }
                NumberAnimation {
                    target: shakeOffset
                    property: "x"
                    to: 0
                    duration: Theme.animFast / 2
                }
            }

            Connections {
                target: Lock

                function onFailed(): void {
                    shake.restart();
                }
            }
        }
    }
}
