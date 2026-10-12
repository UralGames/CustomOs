import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Settings

SettingsPage {
    id: page
    title: qsTr("Taskbar")
    subtitle: qsTr("Alignment, clock and pinned apps.")

    SectionCard {
        Layout.fillWidth: true
        SettingRow {
            title: qsTr("Alignment")
            description: qsTr("Where the Start button and apps sit")
            iconName: "apps"
            ComboBox {
                Layout.preferredWidth: 180
                model: [qsTr("Centre"), qsTr("Left")]
                currentIndex: Settings.taskbarAlignment === "left" ? 1 : 0
                onActivated: (i) => Settings.taskbarAlignment = i === 1 ? "left" : "center"
            }
        }
        Divider {}
        SettingRow {
            title: qsTr("24-hour clock")
            iconName: "clock"
            Switch { checked: Settings.clock24h; onToggled: Settings.clock24h = checked }
        }
        SettingRow {
            title: qsTr("Show seconds")
            iconName: "clock"
            Switch { checked: Settings.showSeconds; onToggled: Settings.showSeconds = checked }
        }
    }

    SectionCard {
        Layout.fillWidth: true
        Heading { text: qsTr("Pinned apps"); level: 3 }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Theme.textSecondary
            text: qsTr("You can also pin and unpin by right-clicking an app on the taskbar or in Start.")
        }
        Repeater {
            model: (Settings.pinnedApps || []).filter(id => Apps.exists(id))
            delegate: RowLayout {
                required property string modelData
                required property int index
                readonly property var app: Apps.get(modelData)
                Layout.fillWidth: true
                spacing: 12
                AppIcon { name: app.iconName; size: 24 }
                Label { text: app.name; Layout.fillWidth: true }
                ToolButton {
                    icon.name: "chevron-up"
                    enabled: index > 0
                    onClicked: {
                        const l = Settings.pinnedApps.slice()
                        l.splice(index - 1, 0, l.splice(index, 1)[0])
                        Settings.pinnedApps = l
                    }
                }
                ToolButton {
                    icon.name: "chevron-down"
                    enabled: index < Settings.pinnedApps.length - 1
                    onClicked: {
                        const l = Settings.pinnedApps.slice()
                        l.splice(index + 1, 0, l.splice(index, 1)[0])
                        Settings.pinnedApps = l
                    }
                }
                Button {
                    text: qsTr("Unpin")
                    onClicked: Settings.pinnedApps = Settings.pinnedApps.filter(id => id !== modelData)
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            ComboBox {
                id: addBox
                Layout.fillWidth: true
                readonly property var candidates: Apps.all().filter(a => (Settings.pinnedApps || []).indexOf(a.appId) < 0)
                model: candidates.map(a => a.name)
            }
            Button {
                text: qsTr("Pin")
                enabled: addBox.candidates.length > 0
                onClicked: Settings.pinnedApps = (Settings.pinnedApps || []).concat([addBox.candidates[addBox.currentIndex].appId])
            }
        }
    }
}
