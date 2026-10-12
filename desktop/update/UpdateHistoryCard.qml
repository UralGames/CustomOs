import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Update

// Installed updates, rollbacks and failures, newest first.
SectionCard {
    id: card
    required property var view
    property bool showAll: false
    readonly property var entries: Updates.history
    readonly property var shown: showAll ? entries : entries.slice(0, 5)

    function resultText(e) {
        switch (e.result) {
        case "success": return e.event === "rollback" ? qsTr("Restored") : qsTr("Installed")
        case "pending": return qsTr("Waiting for restart")
        case "rolled-back": return qsTr("Rolled back")
        case "failed": return qsTr("Failed")
        default: return e.result || ""
        }
    }
    function resultColor(e) {
        switch (e.result) {
        case "success": return Theme.success
        case "rolled-back": return Theme.warning
        case "failed": return Theme.danger
        default: return Theme.textSecondary
        }
    }
    function resultIcon(e) {
        switch (e.result) {
        case "success": return e.event === "rollback" ? "undo" : "check"
        case "pending": return "clock"
        case "rolled-back": return "undo"
        default: return "warning"
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Heading { text: qsTr("Update history"); level: 3; Layout.fillWidth: true }
        Icon { name: "history"; size: 16; color: Theme.textSecondary }
    }

    Label {
        visible: card.entries.length === 0
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: Theme.textSecondary
        text: qsTr("No updates have been installed on this computer yet.")
    }

    Repeater {
        model: card.shown
        delegate: ColumnLayout {
            id: row
            required property var modelData
            required property int index
            Layout.fillWidth: true
            spacing: 10
            Rule { visible: row.index > 0 }
            RowLayout {
                Layout.fillWidth: true
                spacing: 14
                Icon {
                    name: card.resultIcon(row.modelData)
                    size: 18
                    color: card.resultColor(row.modelData)
                    Layout.alignment: Qt.AlignTop
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: row.modelData.event === "rollback"
                              ? qsTr("Back to %1 %2").arg(card.view.osName).arg(row.modelData.to_version || qsTr("the previous version"))
                              : qsTr("%1 %2").arg(card.view.osName).arg(row.modelData.to_version || "")
                              + (row.modelData.from_version ? qsTr("  (from %1)").arg(row.modelData.from_version) : "")
                    }
                    Label {
                        Layout.fillWidth: true
                        visible: (row.modelData.message || "").length > 0
                        text: row.modelData.message || ""
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontCaption
                        wrapMode: Text.Wrap
                    }
                }
                ColumnLayout {
                    spacing: 2
                    Label {
                        Layout.alignment: Qt.AlignRight
                        text: card.resultText(row.modelData)
                        color: card.resultColor(row.modelData)
                        font.weight: Font.Medium
                    }
                    Label {
                        Layout.alignment: Qt.AlignRight
                        text: Qt.formatDateTime(new Date(row.modelData.time), "d MMM yyyy, HH:mm")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontCaption
                    }
                }
            }
        }
    }

    Button {
        visible: card.entries.length > 5
        flat: true
        leftPadding: 0
        text: card.showAll ? qsTr("Show fewer") : qsTr("Show all %1 entries").arg(card.entries.length)
        onClicked: card.showAll = !card.showAll
    }
}
