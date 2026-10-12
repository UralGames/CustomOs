import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Settings

SettingsPage {
    id: page
    title: qsTr("Notifications")
    subtitle: qsTr("Applications send notifications through the standard freedesktop.org service.")

    SectionCard {
        Layout.fillWidth: true
        SettingRow {
            title: qsTr("Do not disturb")
            description: qsTr("Notifications go straight to the notification centre without popping up. Urgent ones still appear.")
            iconName: "bell-off"
            Switch { checked: Settings.doNotDisturb; onToggled: Settings.doNotDisturb = checked }
        }
    }
    SectionCard {
        Layout.fillWidth: true
        Heading { text: qsTr("Tip"); level: 3 }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Theme.textSecondary
            text: qsTr("Open the notification centre by clicking the clock on the taskbar or pressing Super+N. "
                       + "From a terminal you can send a test notification with: notify-send \"Hello\" \"World\"")
        }
    }
}
