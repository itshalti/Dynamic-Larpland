pragma ComponentBehavior: Bound
// Month grid: prev/next month, today in an accent circle, other-month days dimmed.
// Every day is clickable: it becomes the selected day (outlined) and the footer shows it;
// clicking a day of the previous/next month jumps to that month. Mouse wheel changes month.
import QtQuick
import QtQuick.Layouts
import qs.components
import qs.services

Item {
    id: root

    readonly property date today: Time.now
    readonly property int firstDay: Config.calendar.firstDayOfWeek
    readonly property real cellWidth: 54
    readonly property real cellHeight: 40

    // Months relative to the current one.
    property int monthOffset: 0
    readonly property date month: new Date(today.getFullYear(), today.getMonth() + monthOffset, 1)
    readonly property int lead: (month.getDay() - firstDay + 7) % 7
    property date selected: today

    function cellDate(i: int): date {
        return new Date(month.getFullYear(), month.getMonth(), 1 - lead + i);
    }

    function sameDay(a: date, b: date): bool {
        return a.getFullYear() === b.getFullYear() && a.getMonth() === b.getMonth() && a.getDate() === b.getDate();
    }

    function select(day: date): void {
        selected = day;
        monthOffset = (day.getFullYear() - today.getFullYear()) * 12 + day.getMonth() - today.getMonth();
    }

    function selectionText(): string {
        const name = Qt.formatDate(selected, "dddd, d MMMM");
        if (sameDay(selected, today))
            return `Today · ${name}`;
        const days = Math.round((new Date(selected.getFullYear(), selected.getMonth(), selected.getDate()) - new Date(today.getFullYear(), today.getMonth(), today.getDate())) / 86400000);
        if (days === 1)
            return `Tomorrow · ${name}`;
        if (days === -1)
            return `Yesterday · ${name}`;
        return days > 0 ? `${name} · in ${days} days` : `${name} · ${-days} days ago`;
    }

    // Back to the current month and today whenever the view is shown again.
    onVisibleChanged: if (visible) {
        monthOffset = 0;
        selected = today;
    }

    WheelHandler {
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onWheel: event => root.monthOffset += event.angleDelta.y > 0 ? -1 : 1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 4

        RowLayout {
            Layout.fillWidth: true
            Layout.bottomMargin: 6

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
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.select(root.today)
                }
            }

            IconButton {
                icon: "chevron-right"
                size: 36
                baseColor: Theme.surfaceRaised
                onClicked: root.monthOffset++
            }
        }

        // Weekday header and day cells share the same fixed column width, so columns line up.
        Grid {
            Layout.alignment: Qt.AlignHCenter
            columns: 7

            Repeater {
                model: 7

                StyledText {
                    required property int index
                    readonly property int weekday: (root.firstDay + index) % 7

                    width: root.cellWidth
                    height: 26
                    horizontalAlignment: Text.AlignHCenter
                    text: Qt.locale().dayName(weekday, Locale.NarrowFormat)
                    color: weekday === 0 || weekday === 6 ? Theme.weekend : Theme.textSecondary
                    font.pixelSize: Theme.fontSmall
                    font.weight: Font.DemiBold
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
                    readonly property bool isSelected: root.sameDay(date, root.selected)

                    width: root.cellWidth
                    height: root.cellHeight

                    Rectangle {
                        anchors.centerIn: parent
                        width: 34
                        height: 34
                        radius: 17
                        color: cell.isToday ? Theme.accent : area.containsMouse ? Theme.surfaceHover : "transparent"
                        border.width: cell.isSelected && !cell.isToday ? 1.5 : 0
                        border.color: Theme.accent
                        scale: area.pressed ? 0.88 : 1

                        Behavior on color {
                            ColorAnimation {
                                duration: Theme.animFast
                            }
                        }

                        Behavior on scale {
                            NumberAnimation {
                                duration: Theme.animFast
                            }
                        }
                    }

                    StyledText {
                        anchors.centerIn: parent
                        text: cell.date.getDate()
                        color: cell.isToday ? Theme.textOnAccent : cell.inMonth ? Theme.textPrimary : Theme.textTertiary
                        font.weight: cell.isToday || cell.isSelected ? Font.Bold : Font.Normal
                    }

                    MouseArea {
                        id: area
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.select(cell.date)
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            Layout.topMargin: 4
            color: Theme.divider
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 4
            spacing: 8

            Icon {
                name: "calendar"
                size: 15
                opacity: 0.5
            }

            StyledText {
                Layout.fillWidth: true
                text: root.selectionText()
                color: Theme.textSecondary
            }

            StyledText {
                text: "Nothing planned"
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSmall
            }
        }
    }
}
