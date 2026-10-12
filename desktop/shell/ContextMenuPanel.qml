import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Shell

// Menu opened from the taskbar (right-click on an app), drawn in the overlay.
Flyout {
    id: menu
    property var items: []

    width: 280
    height: column.implicitHeight + 12

    ColumnLayout {
        id: column
        x: 6
        y: 6
        width: parent.width - 12
        spacing: 0
        Repeater {
            model: menu.items
            delegate: ColumnLayout {
                required property var modelData
                Layout.fillWidth: true
                spacing: 0
                Rectangle {
                    visible: modelData.separatorBefore === true
                    Layout.fillWidth: true
                    Layout.topMargin: 4
                    Layout.bottomMargin: 4
                    Layout.preferredHeight: 1
                    color: Theme.divider
                }
                ItemDelegate {
                    Layout.fillWidth: true
                    text: modelData.text
                    icon.name: modelData.icon || ""
                    enabled: modelData.enabled !== false
                    onClicked: {
                        menu.closeRequested()
                        if (modelData.trigger) modelData.trigger()
                    }
                }
            }
        }
    }
}
