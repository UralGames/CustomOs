import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Update

// Meridian Update: the Updates page as its own window (also reachable as
// Settings › System › Updates).
ApplicationWindow {
    id: win
    width: 860
    height: 760
    minimumWidth: 560
    minimumHeight: 480
    visible: true
    title: qsTr("Meridian Update")

    property string toast: ""
    property bool toastError: false
    function notify(message, isError) {
        toast = message
        toastError = isError === true
        toastTimer.restart()
    }
    Timer { id: toastTimer; interval: 6000; onTriggered: win.toast = "" }
    Shortcut { sequences: [StandardKey.Quit, StandardKey.Close]; onActivated: Qt.quit() }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: win.toast.length ? 42 : 0
            visible: Layout.preferredHeight > 0
            color: win.toastError ? Theme.dangerBackground : Theme.accentSoft
            Behavior on Layout.preferredHeight { NumberAnimation { duration: Theme.duration } }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 8
                Icon { name: win.toastError ? "warning" : "check"; size: 16; color: win.toastError ? Theme.danger : Theme.accent }
                Label { Layout.fillWidth: true; text: win.toast; elide: Text.ElideRight }
                ToolButton { icon.name: "close"; onClicked: win.toast = "" }
            }
        }

        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            contentHeight: column.implicitHeight + 64
            clip: true

            ColumnLayout {
                id: column
                x: Math.max(24, (scroll.availableWidth - width) / 2)
                width: Math.min(scroll.availableWidth - 48, 820)
                spacing: Theme.spacingL

                Item { Layout.preferredHeight: 12 }
                RowLayout {
                    spacing: 14
                    AppIcon { name: "meridian-update"; size: 40 }
                    ColumnLayout {
                        spacing: 0
                        Heading { text: qsTr("Updates"); level: 1 }
                        Label { text: qsTr("Keep %1 current and secure.").arg(Updates.status.osName || "CustomOS"); color: Theme.textSecondary }
                    }
                }
                UpdatesView {
                    Layout.fillWidth: true
                    notify: win.notify
                }
            }
        }
    }
}
