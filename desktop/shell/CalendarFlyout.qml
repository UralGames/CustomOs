import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Shell

// Notification centre (top) and calendar (bottom), opened from the clock.
Flyout {
    id: fly
    width: 380
    height: col.implicitHeight

    property date shownMonth: new Date()
    onOpenChanged: if (open) shownMonth = new Date()

    ColumnLayout {
        id: col
        width: parent.width
        spacing: 0

        // ---------------- notifications ----------------
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 16
            Layout.bottomMargin: 8
            Heading { text: qsTr("Notifications"); level: 2; font.pixelSize: 15; Layout.fillWidth: true }
            Button {
                text: qsTr("Clear all")
                flat: true
                visible: Notifications.count > 0
                onClicked: Notifications.clearAll()
            }
            ToolButton {
                icon.name: Settings.doNotDisturb ? "bell-off" : "bell"
                checkable: true
                checked: Settings.doNotDisturb
                onToggled: Settings.doNotDisturb = checked
                ToolTip.visible: hovered
                ToolTip.text: Settings.doNotDisturb ? qsTr("Do not disturb is on") : qsTr("Turn on do not disturb")
            }
        }
        ListView {
            id: notifList
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.preferredHeight: count === 0 ? 72 : Math.min(contentHeight, 380)
            clip: true
            spacing: 8
            model: Notifications
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}
            delegate: NotificationCard {
                required property int notificationId
                required property string appName
                required property string appIcon
                required property string summary
                required property string body
                required property var actions
                required property var time
                width: ListView.view.width
                data_: ({ notificationId: notificationId, appName: appName, appIcon: appIcon,
                          summary: summary, body: body, actions: actions, time: time })
                onDismissed: Notifications.dismiss(notificationId)
                onActionClicked: (key) => Notifications.invokeAction(notificationId, key)
            }
            add: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.duration } }
            remove: Transition { NumberAnimation { property: "opacity"; to: 0; duration: Theme.durationFast } }
            displaced: Transition { NumberAnimation { property: "y"; duration: Theme.duration; easing.type: Theme.easing } }
            Label {
                anchors.centerIn: parent
                visible: notifList.count === 0
                text: qsTr("No new notifications")
                color: Theme.textSecondary
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 12
            Layout.preferredHeight: 1
            color: Theme.divider
        }

        // ---------------- calendar ----------------
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 10
            Layout.topMargin: 12
            Label {
                Layout.fillWidth: true
                text: fly.shownMonth.toLocaleDateString(Qt.locale(), "MMMM yyyy")
                font.pixelSize: 15
                font.weight: Font.DemiBold
            }
            ToolButton {
                icon.name: "chevron-up"
                onClicked: fly.shownMonth = new Date(fly.shownMonth.getFullYear(), fly.shownMonth.getMonth() - 1, 1)
            }
            ToolButton {
                icon.name: "chevron-down"
                onClicked: fly.shownMonth = new Date(fly.shownMonth.getFullYear(), fly.shownMonth.getMonth() + 1, 1)
            }
        }
        DayOfWeekRow {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            locale: Qt.locale()
            delegate: Text {
                required property string shortName
                text: shortName.substring(0, 2)
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 12
                font.weight: Font.DemiBold
                color: Theme.textSecondary
            }
        }
        MonthGrid {
            id: grid
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.bottomMargin: 12
            month: fly.shownMonth.getMonth()
            year: fly.shownMonth.getFullYear()
            locale: Qt.locale()
            delegate: Item {
                required property var model
                implicitWidth: 44
                implicitHeight: 40
                readonly property bool isToday: model.today
                Rectangle {
                    anchors.centerIn: parent
                    width: 34
                    height: 34
                    radius: 17
                    color: parent.isToday ? Theme.accent : dayMouse.containsMouse ? Theme.hover : "transparent"
                }
                Text {
                    anchors.centerIn: parent
                    text: model.day
                    font.pixelSize: 13
                    font.weight: parent.isToday ? Font.DemiBold : Font.Normal
                    color: parent.isToday ? Theme.onAccent
                         : model.month === grid.month ? Theme.text : Theme.textTertiary
                }
                MouseArea { id: dayMouse; anchors.fill: parent; hoverEnabled: true }
            }
        }
    }
}
