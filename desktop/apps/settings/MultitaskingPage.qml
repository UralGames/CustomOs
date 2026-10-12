import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Settings

SettingsPage {
    id: page
    title: qsTr("Multitasking")
    subtitle: qsTr("Virtual desktops, snapping and keyboard shortcuts.")

    SectionCard {
        Layout.fillWidth: true
        SettingRow {
            title: qsTr("Number of desktops")
            description: qsTr("Switch with Ctrl+Super+←/→ or from Task view (Super+Tab)")
            iconName: "desktops"
            SpinBoxLike {
                value: Settings.desktopCount
                from: 1
                to: 9
                onModified: (v) => Settings.desktopCount = v
            }
        }
    }

    SectionCard {
        Layout.fillWidth: true
        Heading { text: qsTr("Snap windows"); level: 3 }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Theme.textSecondary
            text: qsTr("Drag a window by its title bar to the left or right edge to fill half of the screen, "
                       + "to a corner for a quarter, or to the top edge to maximise it. With the keyboard, "
                       + "Super+← and Super+→ snap to the halves, Super+↑ maximises and Super+↓ restores.")
        }
    }

    SectionCard {
        Layout.fillWidth: true
        Heading { text: qsTr("Keyboard shortcuts"); level: 3 }
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 24
            rowSpacing: 6
            Repeater {
                model: [
                    ["Super", qsTr("Open Start")],
                    ["Super + A", qsTr("Quick settings")],
                    ["Super + N", qsTr("Notifications and calendar")],
                    ["Super + Tab", qsTr("Task view")],
                    ["Alt + Tab", qsTr("Switch windows")],
                    ["Super + D", qsTr("Show the desktop")],
                    ["Super + L", qsTr("Lock the screen")],
                    ["Super + E", qsTr("Files")],
                    ["Super + Enter  /  Ctrl + Alt + T", qsTr("Terminal")],
                    ["Super + I", qsTr("Settings")],
                    ["Ctrl + Shift + Esc", qsTr("System Monitor")],
                    ["Super + ↑", qsTr("Maximise window")],
                    ["Super + ↓", qsTr("Restore window")],
                    ["Super + H", qsTr("Minimise window")],
                    ["Super + ← / →", qsTr("Snap window left / right")],
                    ["Alt + F4", qsTr("Close window")],
                    ["Alt + Space", qsTr("Window menu")],
                    ["Ctrl + Super + ← / →", qsTr("Previous / next desktop")],
                    ["Shift + Super + ← / →", qsTr("Move window to previous / next desktop")],
                    ["Super + 1 … 9", qsTr("Open or switch to the n-th taskbar app")],
                    ["Print  /  Super + Shift + S", qsTr("Screenshot (saved to Pictures/Screenshots)")]
                ]
                delegate: RowLayout {
                    required property var modelData
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    spacing: 24
                    Rectangle {
                        Layout.preferredWidth: 260
                        implicitHeight: keyLabel.implicitHeight + 8
                        radius: Theme.radiusSmall
                        color: Theme.surfaceAlt
                        Label { id: keyLabel; anchors.centerIn: parent; text: modelData[0]; font.family: Theme.monoFamily; font.pixelSize: 12 }
                    }
                    Label { text: modelData[1]; Layout.fillWidth: true }
                }
            }
        }
    }
}
