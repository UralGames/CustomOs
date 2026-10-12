import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Settings

SettingsPage {
    id: page
    title: qsTr("Date & time")
    subtitle: qsTr("Time zone and clock synchronisation (systemd-timedated).")

    property date now: new Date()
    Timer { interval: 1000; running: true; repeat: true; onTriggered: page.now = new Date() }
    property var zones: []
    Component.onCompleted: { SystemActions.refreshTime(); zones = SystemActions.timezones() }

    Connections {
        target: SystemActions
        function onFinished(action, ok, message) {
            if (action === "timezone" || action === "ntp") page.notify(message, !ok)
        }
    }

    SectionCard {
        Layout.fillWidth: true
        Label { text: Qt.formatTime(page.now, Settings.clock24h ? "HH:mm:ss" : "h:mm:ss AP"); font.pixelSize: Theme.fontDisplay; font.weight: Font.DemiBold }
        Label { text: page.now.toLocaleDateString(Qt.locale(), Locale.LongFormat); color: Theme.textSecondary }
    }

    SectionCard {
        Layout.fillWidth: true
        SettingRow {
            title: qsTr("Time zone")
            iconName: "globe"
            ComboBox {
                id: zoneBox
                Layout.preferredWidth: 320
                model: page.zones
                currentIndex: page.zones.indexOf(SystemActions.timezone)
                onActivated: (i) => SystemActions.setTimezone(page.zones[i])
            }
        }
        TextField {
            Layout.fillWidth: true
            placeholderText: qsTr("Find a time zone (type a city, e.g. Istanbul)")
            onTextChanged: {
                const t = text.trim().toLowerCase()
                if (!t.length) return
                const i = page.zones.findIndex(z => z.toLowerCase().indexOf(t) >= 0)
                if (i >= 0) zoneBox.currentIndex = i
            }
            onAccepted: if (zoneBox.currentIndex >= 0) SystemActions.setTimezone(page.zones[zoneBox.currentIndex])
        }
        Divider {}
        SettingRow {
            title: qsTr("Set time automatically")
            description: qsTr("Synchronise the clock with internet time servers when online")
            iconName: "clock"
            Switch { checked: SystemActions.ntp; onToggled: SystemActions.setNtp(checked) }
        }
        SettingRow {
            title: qsTr("24-hour clock")
            iconName: "clock"
            Switch { checked: Settings.clock24h; onToggled: Settings.clock24h = checked }
        }
    }
}
