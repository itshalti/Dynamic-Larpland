pragma ComponentBehavior: Bound
// Month name + a strip of days around today. Clicking opens the month calendar.
import QtQuick
import QtQuick.Layouts
import qs.components
import qs.services

Item {
    id: root

    readonly property date today: Time.now
    readonly property int span: 3

    signal openCalendar

    function dayAt(offset: int): date {
        return new Date(today.getFullYear(), today.getMonth(), today.getDate() + offset);
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.openCalendar()
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 14

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 12

            StyledText {
                Layout.alignment: Qt.AlignBottom
                Layout.bottomMargin: 2
                text: Qt.locale().standaloneMonthName(root.today.getMonth(), Locale.ShortFormat)
                font.pixelSize: Theme.fontHuge
                font.weight: Font.Bold
            }

            Repeater {
                model: root.span * 2 + 1

                Column {
                    id: day

                    required property int index
                    readonly property int offset: index - root.span
                    readonly property date date: root.dayAt(offset)
                    readonly property bool isToday: offset === 0
                    readonly property bool isWeekend: date.getDay() === 0 || date.getDay() === 6

                    Layout.alignment: Qt.AlignBottom
                    spacing: 2
                    opacity: 1 - Math.abs(offset) * 0.14

                    StyledText {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: day.isToday ? Qt.locale().dayName(day.date.getDay(), Locale.ShortFormat).toUpperCase() : Qt.locale().dayName(day.date.getDay(), Locale.NarrowFormat)
                        color: day.isWeekend && !day.isToday ? Theme.weekend : Theme.textSecondary
                        font.pixelSize: Theme.fontSmall
                        font.weight: day.isToday ? Font.Bold : Font.Medium
                    }

                    StyledText {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: String(day.date.getDate()).padStart(2, "0")
                        color: day.isToday ? Theme.accent : day.isWeekend ? Theme.weekend : Theme.textSecondary
                        font.pixelSize: day.isToday ? 28 : Theme.fontTitle
                        font.weight: day.isToday ? Font.Bold : Font.Medium
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
                text: "Nothing for today"
                color: Theme.textSecondary
                font.pixelSize: Theme.fontLarge
            }
        }
    }
}
