import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Settings

SettingsPage {
    id: page
    title: qsTr("Display")
    subtitle: qsTr("Resolution and scale apply immediately and are restored when you sign in.")

    Component.onCompleted: Display.refresh()

    // After a change, ask to keep it; revert automatically after 15 seconds
    // so a mode the screen cannot show never leaves you stuck.
    property var revert: null
    property int countdown: 0
    function changed(revertFn) {
        revert = revertFn
        countdown = 15
        keepDialog.open()
    }
    Timer {
        interval: 1000
        repeat: true
        running: keepDialog.visible
        onTriggered: {
            page.countdown -= 1
            if (page.countdown <= 0) { keepDialog.close(); if (page.revert) page.revert(); page.revert = null }
        }
    }

    Label {
        visible: !Display.available
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: Theme.textSecondary
        text: qsTr("Display information is not available (the compositor's output-management interface or wlr-randr is missing).")
    }

    Repeater {
        model: Display.outputs
        delegate: SectionCard {
            id: card
            required property var modelData
            Layout.fillWidth: true
            readonly property var modes: modelData.modes || []
            // Unique resolutions, largest first
            readonly property var choices: {
                const seen = {}, out = []
                const sorted = modes.slice().sort((a, b) => (b.width * b.height - a.width * a.height) || (b.refresh - a.refresh))
                for (const m of sorted) {
                    const k = m.width + "x" + m.height
                    if (seen[k]) continue
                    seen[k] = true
                    out.push(m)
                }
                return out
            }
            readonly property int currentIndex: choices.findIndex(m => m.current
                                                || (modes.find(x => x.current) && modes.find(x => x.current).width === m.width
                                                    && modes.find(x => x.current).height === m.height))

            RowLayout {
                Layout.fillWidth: true
                Icon { name: "display"; size: 22 }
                ColumnLayout {
                    spacing: 0
                    Label { text: card.modelData.name; font.weight: Font.DemiBold }
                    Label { text: card.modelData.description; color: Theme.textSecondary; font.pixelSize: Theme.fontCaption }
                }
            }
            Divider {}
            SettingRow {
                title: qsTr("Display resolution")
                description: qsTr("Virtual machines usually offer many sizes; 1920 × 1080 suits most screens.")
                iconName: "display"
                ComboBox {
                    Layout.preferredWidth: 260
                    model: card.choices.map(m => m.label + (m.preferred ? qsTr(" (recommended)") : ""))
                    currentIndex: card.currentIndex
                    enabled: !Display.busy
                    onActivated: (i) => {
                        const m = card.choices[i]
                        const prev = card.modes.find(x => x.current)
                        Display.setMode(card.modelData.name, m.id)
                        if (prev) page.changed(() => Display.setMode(card.modelData.name, prev.id))
                    }
                }
            }
            Divider {}
            SettingRow {
                title: qsTr("Scale")
                description: qsTr("Makes text, apps and other items bigger.")
                iconName: "maximize"
                ComboBox {
                    id: scaleBox
                    Layout.preferredWidth: 260
                    readonly property var values: [1.0, 1.25, 1.5, 1.75, 2.0]
                    model: values.map(v => Math.round(v * 100) + "%" + (v === 1.0 ? qsTr(" (recommended)") : ""))
                    currentIndex: Math.max(0, values.findIndex(v => Math.abs(v - card.modelData.scale) < 0.01))
                    enabled: !Display.busy
                    onActivated: (i) => {
                        const prev = card.modelData.scale
                        Display.setScale(card.modelData.name, values[i])
                        page.changed(() => Display.setScale(card.modelData.name, prev))
                    }
                }
            }
        }
    }

    Connections {
        target: Display
        function onApplied(ok, message) { if (!ok) page.notify(message, true) }
    }

    Dialog {
        id: keepDialog
        title: qsTr("Keep these display settings?")
        width: 440
        closePolicy: Popup.NoAutoClose
        contentItem: Label {
            text: qsTr("Reverting to the previous settings in %1 seconds.").arg(page.countdown)
            wrapMode: Text.Wrap
            color: Theme.textSecondary
        }
        footer: DialogButtonBox {
            Button { text: qsTr("Revert"); DialogButtonBox.buttonRole: DialogButtonBox.RejectRole }
            Button { text: qsTr("Keep changes"); DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole }
        }
        onAccepted: page.revert = null
        onRejected: { if (page.revert) page.revert(); page.revert = null }
    }
}
