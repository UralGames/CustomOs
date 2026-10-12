import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Update

// The content of Settings › System › Updates, also used by the Meridian
// Update app. Everything shown comes from the update service
// (customos-updated) and every button asks it to act.
ColumnLayout {
    id: view
    spacing: Theme.spacingL

    // The host's message strip (Settings passes its toast function).
    property var notify: function(message, isError) {}

    readonly property var s: Updates.status
    readonly property string state: !Updates.serviceAvailable ? "unavailable" : (s.state || "idle")
    readonly property bool working: ["checking", "downloading", "verifying", "installing", "confirming"].indexOf(state) >= 0
    readonly property bool canAct: Updates.administrator && !Updates.requestPending
    readonly property string osName: s.osName || "CustomOS"

    function bytes(n) { return Sys.formatBytes(Number(n) || 0) }

    function when(secs) {
        if (!secs)
            return qsTr("never")
        const d = new Date(Number(secs) * 1000)
        const now = new Date()
        const time = Qt.formatTime(d, "HH:mm")
        if (d.toDateString() === now.toDateString())
            return qsTr("today at %1").arg(time)
        const yesterday = new Date(now.getTime() - 86400000)
        if (d.toDateString() === yesterday.toDateString())
            return qsTr("yesterday at %1").arg(time)
        return Qt.formatDate(d, "d MMMM yyyy")
    }

    function timeLeft() {
        const rate = Number(s.downloadRate) || 0
        const left = (Number(s.downloadSize) || 0) - (Number(s.bytesDownloaded) || 0)
        if (rate <= 0 || left <= 0)
            return ""
        const secs = left / rate
        if (secs < 60)
            return qsTr("less than a minute left")
        if (secs < 3600)
            return qsTr("about %n minute(s) left", "", Math.round(secs / 60))
        return qsTr("about %n hour(s) left", "", Math.round(secs / 3600))
    }

    Connections {
        target: Updates
        function onActionFailed(message) { view.notify(message, true) }
    }

    UpdateStatusCard {
        Layout.fillWidth: true
        view: view
        onRestartRequested: restartDialog.open()
    }

    // Why this computer can't install updates (live image, 0.1 layout).
    InfoStrip {
        Layout.fillWidth: true
        visible: Updates.serviceAvailable && view.s.supported === false
        iconName: "info"
        text: view.s.unsupportedMessage || ""
    }
    InfoStrip {
        Layout.fillWidth: true
        visible: Updates.serviceAvailable && !Updates.administrator && view.s.supported !== false
        iconName: "lock"
        text: qsTr("You can see the update status, but only administrators can download and install updates.")
    }

    ReleaseNotesCard {
        Layout.fillWidth: true
        view: view
        visible: Updates.serviceAvailable && view.s.updateAvailable === true && (view.s.releaseNotes || "").length > 0
    }

    UpdateDetailsCard {
        Layout.fillWidth: true
        view: view
        visible: Updates.serviceAvailable
    }

    // Going back to the version in the other slot.
    SectionCard {
        Layout.fillWidth: true
        visible: Updates.serviceAvailable && view.s.rollbackAvailable === true
        Heading { text: qsTr("Previous version"); level: 3 }
        RowLayout {
            Layout.fillWidth: true
            spacing: 14
            Icon { name: "undo"; size: 18; color: Theme.textSecondary }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: view.s.otherSlotVersion
                          ? qsTr("Go back to %1 %2").arg(view.osName).arg(view.s.otherSlotVersion)
                          : qsTr("Go back to the previous version")
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontCaption
                    text: qsTr("The previous version is still installed. Your files and settings are kept.")
                }
            }
            Button {
                text: qsTr("Go back…")
                enabled: view.canAct && !view.working
                onClicked: rollbackDialog.open()
            }
        }
    }

    UpdateHistoryCard {
        Layout.fillWidth: true
        view: view
        visible: Updates.serviceAvailable
    }

    ConfirmDialog {
        id: restartDialog
        title: qsTr("Restart now?")
        text: qsTr("Save your work first: open applications will close. %1 restarts and starts the updated version.")
              .arg(view.osName)
        acceptText: qsTr("Restart now")
        onAccepted: Updates.reboot()
    }

    ConfirmDialog {
        id: rollbackDialog
        title: qsTr("Go back to the previous version?")
        text: qsTr("After the next restart your computer starts %1 %2 again. Your files and settings are kept. "
                   + "You can install the newer version again later.").arg(view.osName).arg(view.s.otherSlotVersion || "")
        acceptText: qsTr("Go back")
        onAccepted: Updates.rollback()
    }
}
