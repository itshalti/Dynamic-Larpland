pragma ComponentBehavior: Bound
// Month grid: prev/next month, today in an accent circle, other-month days dimmed.
import QtQuick
import QtQuick.Layouts
import qs.components
import qs.services

Item {
    id: root

    readonly property date today: Time.now
    readonly property int firstDay: Config.calendar.firstDayOfWeek
    // Months relative to the current one.
    property int monthOffset: 0
    readonly property date month: new Date(today.getFullYear(), today.getMonth() + monthOffset, 1)
    readonly property int lead: (month.getDay() - firstDay + 7) % 7

    function cellDate(i: int): date {
        return new Date(month.getFullYear(), month.getMonth(), 1 - lead + i);
    }

    function sameDay(a: date, b: date): bool {
        return a.getFullYear() === b.getFullYear() && a.getMonth() === b.getMonth() && a.getDate() === b.getDate();
    }

    // Reset to the current month whenever the view is shown again.
    onVisibleChanged: if (visible)
        monthOffset = 0

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            Layout.bottomMargin: 8

            IconButton {
                icon: "chevron-left"
                size: 36
                baseColor: Theme.surfaceRaised
                onClicked: root.monthOffset--
            }

            StyledText {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: Qt.locale().standaloneMonthName(root.month.getMonth()) + " " + root.month.getFullYear()
                font.pixelSize: Theme.fontLarge
                font.weight: Font.Bold

                MouseArea {
                    anchors.fill: parent
                    cursorShape: root.monthOffset !== 0 ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: root.monthOffset = 0
                }
            }

            IconButton {
                icon: "chevron-right"
                size: 36
                baseColor: Theme.surfaceRaised
                onClicked: root.monthOffset++
            }
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: 7
            rowSpacing: 0
            columnSpacing: 0

            Repeater {
                model: 7

                StyledText {
                    required property int index
                    readonly property int weekday: (root.firstDay + index) % 7

                    Layout.fillWidth: true
                    Layout.preferredHeight: 28
                    horizontalAlignment: Text.AlignHCenter
                    text: Qt.locale().dayName(weekday, Locale.NarrowFormat)
                    color: Theme.textSecondary
                    font.weight: Font.Medium
                }
            }

            Repeater {
                model: 42

                Item {
                    id: cell

                    required property int index
                    readonly property date date: root.cellDate(index)
                    readonly property bool inMonth: date.getMonth() === root.month.getMonth()
                    readonly property bool isToday: root.sameDay(date, root.today)

                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Rectangle {
                        anchors.centerIn: parent
                        width: 34
                        height: 34
                        radius: 17
                        color: Theme.accent
                        visible: cell.isToday
                    }

                    StyledText {
                        anchors.centerIn: parent
                        text: cell.date.getDate()
                        color: cell.isToday ? Theme.textOnAccent : cell.inMonth ? Theme.textPrimary : Theme.textTertiary
                        font.weight: cell.isToday ? Font.Bold : Font.Normal
                    }
                }
            }
        }
    }
}
