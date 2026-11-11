pragma ComponentBehavior: Bound
// Month name + a carousel of days. The mouse wheel spins the carousel one day per notch
// (down = forward, up = back); the day in the middle is the focused one and grows.
// Clicking opens the month calendar. Resets to today whenever it's shown again.
import QtQuick
import QtQuick.Layouts
import qs.components
import qs.services

Item {
    id: root

    readonly property date today: Time.now
    // Days visible on each side of the centre.
    readonly property int span: 3
    readonly property real cellWidth: 36

    // Target offset from today in days, and its animated value that drives the layout.
    property int target: 0
    property real shift: target
    readonly property date focused: dayAt(target)
    readonly property int base: Math.floor(shift)

    property real wheelAccum: 0

    signal openCalendar

    function dayAt(offset: int): date {
        return new Date(today.getFullYear(), today.getMonth(), today.getDate() + offset);
    }

    Behavior on shift {
        NumberAnimation {
            duration: 260
            easing.type: Easing.OutCubic
        }
    }

    onVisibleChanged: if (!visible) {
        target = 0;
        wheelAccum = 0;
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.openCalendar()
        onWheel: wheel => {
            // 120 units = one notch; touchpads send smaller steps, so accumulate.
            root.wheelAccum += wheel.angleDelta.y !== 0 ? wheel.angleDelta.y : wheel.angleDelta.x;
            while (Math.abs(root.wheelAccum) >= 120) {
                const step = root.wheelAccum > 0 ? -1 : 1;
                root.target += step;
                root.wheelAccum += step * 120;
            }
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 12

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 10

            StyledText {
                Layout.alignment: Qt.AlignBottom
                Layout.bottomMargin: 4
                Layout.preferredWidth: 66
                horizontalAlignment: Text.AlignRight
                text: Qt.locale().standaloneMonthName(root.focused.getMonth(), Locale.ShortFormat)
                font.pixelSize: Theme.fontHuge
                font.weight: Font.Bold
            }

            // The carousel
            Item {
                id: track

                Layout.preferredWidth: root.cellWidth * (root.span * 2 + 1)
                Layout.preferredHeight: 58
                clip: true

                Repeater {
                    // One spare item on each side so days slide in instead of popping.
                    model: root.span * 2 + 3

                    Column {
                        id: day

                        required property int index
                        readonly property int offset: root.base + index - root.span - 1
                        // Continuous distance from the centre slot, in days.
                        readonly property real distance: offset - root.shift
                        readonly property real closeness: Math.max(0, 1 - Math.abs(distance))
                        readonly property date date: root.dayAt(offset)
                        readonly property bool isToday: offset === 0
                        readonly property bool isWeekend: date.getDay() === 0 || date.getDay() === 6

                        x: track.width / 2 + distance * root.cellWidth - width / 2
                        anchors.bottom: parent.bottom
                        width: root.cellWidth
                        spacing: 2
                        opacity: Math.max(0, 1 - Math.max(0, Math.abs(distance) - 0.5) * 0.3)
                        scale: 1 + closeness * 0.22
                        transformOrigin: Item.Bottom

                        StyledText {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: day.closeness > 0.5 ? Qt.locale().dayName(day.date.getDay(), Locale.ShortFormat).toUpperCase() : Qt.locale().dayName(day.date.getDay(), Locale.NarrowFormat)
                            color: day.isWeekend && day.closeness <= 0.5 ? Theme.weekend : Theme.textSecondary
                            font.pixelSize: Theme.fontSmall - 1
                            font.weight: day.closeness > 0.5 ? Font.Bold : Font.Medium
                        }

                        StyledText {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: String(day.date.getDate()).padStart(2, "0")
                            color: day.isToday ? Theme.accent : day.closeness > 0.5 ? Theme.textPrimary : day.isWeekend ? Theme.weekend : Theme.textSecondary
                            font.pixelSize: Theme.fontTitle
                            font.weight: day.closeness > 0.5 || day.isToday ? Font.Bold : Font.Medium
                        }
                    }
                }
            }
        }

        Row {
            Layout.alignment: Qt.AlignHCenter
            spacing: 8

            Icon {
                anchors.verticalCenter: parent.verticalCenter
                name: "calendar"
                size: 16
                opacity: 0.5
            }

            StyledText {
                anchors.verticalCenter: parent.verticalCenter
                text: root.target === 0 ? "Nothing for today" : root.target === 1 ? "Nothing for tomorrow" : root.target === -1 ? "Nothing yesterday" : "Nothing on " + Qt.formatDate(root.focused, "d MMM")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontLarge
            }
        }
    }
}
