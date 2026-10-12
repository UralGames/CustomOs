import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Settings

SettingsPage {
    id: page
    title: qsTr("Keyboard & mouse")
    subtitle: qsTr("These settings apply to the whole session as soon as you change them.")

    readonly property var layouts: SystemActions.keyboardLayouts()
    readonly property var variants: SystemActions.keyboardVariants(Settings.keyboardLayout)

    SectionCard {
        Layout.fillWidth: true
        Heading { text: qsTr("Keyboard"); level: 3 }
        SettingRow {
            title: qsTr("Layout")
            iconName: "keyboard"
            ComboBox {
                Layout.preferredWidth: 300
                model: page.layouts.map(l => l.name)
                currentIndex: page.layouts.findIndex(l => l.code === Settings.keyboardLayout)
                onActivated: (i) => { Settings.keyboardVariant = ""; Settings.keyboardLayout = page.layouts[i].code }
            }
        }
        SettingRow {
            title: qsTr("Variant")
            iconName: "keyboard"
            ComboBox {
                Layout.preferredWidth: 300
                model: page.variants.map(v => v.name)
                currentIndex: Math.max(0, page.variants.findIndex(v => v.code === Settings.keyboardVariant))
                onActivated: (i) => Settings.keyboardVariant = page.variants[i].code
            }
        }
        Divider {}
        SettingRow {
            title: qsTr("Repeat delay")
            description: qsTr("How long to hold a key before it repeats")
            Slider {
                Layout.preferredWidth: 240
                from: 150; to: 1000; stepSize: 25
                value: Settings.repeatDelay
                onPressedChanged: if (!pressed) Settings.repeatDelay = value
            }
            Label { text: Settings.repeatDelay + " ms"; Layout.preferredWidth: 64; color: Theme.textSecondary }
        }
        SettingRow {
            title: qsTr("Repeat rate")
            description: qsTr("Characters per second while a key is held")
            Slider {
                Layout.preferredWidth: 240
                from: 5; to: 60; stepSize: 1
                value: Settings.repeatRate
                onPressedChanged: if (!pressed) Settings.repeatRate = value
            }
            Label { text: Settings.repeatRate + "/s"; Layout.preferredWidth: 64; color: Theme.textSecondary }
        }
        TextField {
            Layout.fillWidth: true
            placeholderText: qsTr("Type here to test the keyboard")
        }
    }

    SectionCard {
        Layout.fillWidth: true
        Heading { text: qsTr("Mouse and touchpad"); level: 3 }
        SettingRow {
            title: qsTr("Pointer speed")
            iconName: "mouse"
            Label { text: qsTr("Slow"); color: Theme.textSecondary }
            Slider {
                Layout.preferredWidth: 220
                from: -1; to: 1; stepSize: 0.1
                value: Settings.pointerSpeed
                onPressedChanged: if (!pressed) Settings.pointerSpeed = Math.round(value * 10) / 10
            }
            Label { text: qsTr("Fast"); color: Theme.textSecondary }
        }
        SettingRow {
            title: qsTr("Natural scrolling")
            description: qsTr("Content follows your fingers, like on a phone")
            iconName: "mouse"
            Switch { checked: Settings.naturalScroll; onToggled: Settings.naturalScroll = checked }
        }
        SettingRow {
            title: qsTr("Left-handed buttons")
            description: qsTr("Swap the primary and secondary buttons")
            iconName: "mouse"
            Switch { checked: Settings.leftHanded; onToggled: Settings.leftHanded = checked }
        }
        SettingRow {
            title: qsTr("Tap to click")
            description: qsTr("Touchpads only")
            iconName: "mouse"
            Switch { checked: Settings.tapToClick; onToggled: Settings.tapToClick = checked }
        }
    }
}
