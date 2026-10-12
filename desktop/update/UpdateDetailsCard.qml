import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core

// Facts about the installed system and where updates come from.
SectionCard {
    id: card
    required property var view
    readonly property var s: view.s

    Heading { text: qsTr("This computer"); level: 3 }

    GridLayout {
        Layout.fillWidth: true
        columns: 2
        columnSpacing: 24
        rowSpacing: 10

        Label { text: qsTr("Installed version"); color: Theme.textSecondary; Layout.preferredWidth: 170 }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: (card.s.prettyName || card.view.osName) + (card.s.buildId ? qsTr(" (build %1)").arg(card.s.buildId) : "")
        }

        Label { text: qsTr("System slot"); color: Theme.textSecondary; visible: (card.s.bootedSlot || "").length > 0 && card.s.mode === "ab" }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            visible: (card.s.bootedSlot || "").length > 0 && card.s.mode === "ab"
            text: card.s.otherSlotVersion
                  ? qsTr("%1 (slot %2 keeps %3 %4)").arg(card.s.bootedSlot).arg(card.s.otherSlot).arg(card.view.osName).arg(card.s.otherSlotVersion)
                  : (card.s.bootedSlot || "")
        }

        Label { text: qsTr("Last successful update"); color: Theme.textSecondary }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: card.s.lastSuccessVersion
                  ? qsTr("%1 %2, %3").arg(card.view.osName).arg(card.s.lastSuccessVersion).arg(card.view.when(card.s.lastSuccessTime))
                  : qsTr("None yet")
        }

        Label { text: qsTr("Last check"); color: Theme.textSecondary }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Label { text: card.view.when(card.s.lastCheck); Layout.fillWidth: true }
            Label {
                Layout.fillWidth: true
                visible: (card.s.lastCheckError || "").length > 0 && card.view.state !== "error"
                text: card.s.lastCheckError || ""
                color: Theme.warning
                font.pixelSize: Theme.fontCaption
                wrapMode: Text.Wrap
            }
        }

        Label { text: qsTr("Update source"); color: Theme.textSecondary }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WrapAnywhere
            text: card.s.repository
                  ? qsTr("github.com/%1 • %2 releases").arg(card.s.repository).arg(card.s.channel || "stable")
                  : qsTr("Not configured")
        }

        Label { text: qsTr("Automatic checks"); color: Theme.textSecondary }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: card.s.automaticCheck ? qsTr("On — new versions are announced; nothing is installed without you")
                                        : qsTr("Off")
        }
    }

    Rule {}

    RowLayout {
        Layout.fillWidth: true
        spacing: 12
        Icon { name: "shield-check"; size: 18; color: Theme.success; Layout.alignment: Qt.AlignTop }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Theme.textSecondary
            font.pixelSize: Theme.fontCaption
            text: qsTr("Updates are checked against CustomOS's signing key before they are installed, and they are "
                       + "written next to the running system, never over it. If a new version doesn't start, "
                       + "your computer goes back to the previous one automatically.")
        }
    }
}
