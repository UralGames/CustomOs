import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Settings

SettingsPage {
    id: page
    title: qsTr("Sound")
    subtitle: qsTr("Output device and volume (PipeWire).")

    Component.onCompleted: Audio.active = true
    Component.onDestruction: Audio.active = false

    SectionCard {
        Layout.fillWidth: true
        visible: !Audio.available
        RowLayout {
            Icon { name: "volume-mute"; size: 20; color: Theme.textSecondary }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("The sound service is not running, or this computer has no sound device. "
                           + "In a virtual machine, enable audio in its settings (for example Intel HD Audio).")
                color: Theme.textSecondary
            }
        }
    }

    SectionCard {
        Layout.fillWidth: true
        visible: Audio.available
        Heading { text: qsTr("Output"); level: 3 }
        Repeater {
            model: Audio.sinks
            delegate: RadioButton {
                required property var modelData
                Layout.fillWidth: true
                text: modelData.name
                checked: modelData.isDefault
                onClicked: Audio.setDefaultSink(modelData.id)
            }
        }
        Label {
            visible: Audio.sinks.length === 0
            text: qsTr("No output devices were found.")
            color: Theme.textSecondary
        }
        Divider {}
        SettingRow {
            title: qsTr("Volume")
            iconName: Audio.muted ? "volume-mute" : "volume-high"
            Slider {
                Layout.preferredWidth: 280
                from: 0; to: 100; stepSize: 1
                value: Audio.volume
                onMoved: Audio.setVolume(value)
            }
            Label { text: Audio.volume + "%"; Layout.preferredWidth: 44; color: Theme.textSecondary }
        }
        SettingRow {
            title: qsTr("Mute")
            iconName: "volume-mute"
            Switch { checked: Audio.muted; onToggled: Audio.setMuted(checked) }
        }
    }
}
