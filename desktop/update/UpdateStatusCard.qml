import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Update

// The headline of the Updates page: what is going on, and the one or two
// actions that make sense right now.
Rectangle {
    id: card
    required property var view
    signal restartRequested()

    readonly property var s: view.s
    readonly property string st: view.state
    readonly property bool hasProgress: ["downloading", "verifying", "installing", "confirming"].indexOf(st) >= 0
    // States that offer buttons (decided from the state, not from the
    // buttons: children of a hidden row never count as visible).
    readonly property bool hasActions: ["checking", "verifying", "installing", "confirming"].indexOf(st) < 0
    readonly property bool isError: st === "error"
    readonly property bool rolledBack: isError && s.errorCode === "rolled-back"
    property bool showDetail: false

    color: Theme.surface
    radius: Theme.radiusLarge
    border.color: isError ? Theme.alpha(tone, 0.45) : Theme.border
    implicitHeight: column.implicitHeight + 2 * Theme.spacingXL

    readonly property string iconName: {
        switch (st) {
        case "unavailable": return "warning"
        case "checking": return "refresh"
        case "up-to-date": return "check"
        case "update-available": return "downloads"
        case "downloading": return "downloads"
        case "verifying": return "shield-check"
        case "installing": return "update"
        case "confirming": return "shield-check"
        case "reboot-required": return "restart"
        case "error": return rolledBack ? "undo" : "warning"
        default: return "update"
        }
    }
    readonly property color tone: st === "up-to-date" ? Theme.success
                                  : st === "error" ? (rolledBack ? Theme.warning : Theme.danger)
                                  : st === "unavailable" ? Theme.warning
                                  : Theme.accent

    readonly property string title: {
        const v = s.availableVersion || ""
        switch (st) {
        case "unavailable": return qsTr("Updates aren't available")
        case "checking": return qsTr("Checking for updates…")
        case "up-to-date": return qsTr("You're up to date")
        case "update-available": return qsTr("%1 %2 is available").arg(view.osName).arg(v)
        case "downloading": return qsTr("Downloading %1 %2").arg(view.osName).arg(v)
        case "verifying": return qsTr("Verifying %1 %2").arg(view.osName).arg(v)
        case "installing": return qsTr("Installing %1 %2").arg(view.osName).arg(v)
        case "confirming": return qsTr("Finishing the update")
        case "reboot-required":
            return s.pendingKind === "rollback" ? qsTr("Restart to go back to the previous version")
                                                : qsTr("Restart to finish the update")
        case "error":
            switch (s.errorCode) {
            case "rolled-back": return qsTr("The update was undone")
            case "signature-invalid": return qsTr("Update blocked")
            case "offline": case "rate-limited": case "server-error": case "tls":
                return qsTr("Couldn't check for updates")
            case "download-failed": case "no-space": return qsTr("The download didn't finish")
            default: return qsTr("The update didn't finish")
            }
        default: return view.osName + " " + (s.currentVersion || "")
        }
    }

    readonly property string subtitle: {
        switch (st) {
        case "unavailable":
            return qsTr("The update service isn't running on this system.")
        case "checking":
            return s.repository ? qsTr("Looking for new releases of github.com/%1").arg(s.repository) : ""
        case "up-to-date":
            return qsTr("%1 %2 • Last checked %3").arg(view.osName).arg(s.currentVersion || "").arg(view.when(s.lastCheck))
        case "update-available": {
            const parts = []
            if (s.releaseDate)
                parts.push(qsTr("Released %1").arg(Qt.formatDate(new Date(Number(s.releaseDate) * 1000), "d MMMM yyyy")))
            parts.push(qsTr("%1 download").arg(view.bytes(s.downloadSize)))
            if (s.downloaded)
                parts.push(qsTr("downloaded and verified"))
            parts.push(qsTr("you have %1").arg(s.currentVersion || "?"))
            return parts.join(" • ")
        }
        case "downloading": {
            const parts = [qsTr("%1 of %2").arg(view.bytes(s.bytesDownloaded)).arg(view.bytes(s.downloadSize))]
            if (Number(s.downloadRate) > 0)
                parts.push(qsTr("%1/s").arg(view.bytes(s.downloadRate)))
            const left = view.timeLeft()
            if (left.length)
                parts.push(left)
            return parts.join(" • ")
        }
        case "verifying":
            return (s.progressText || qsTr("Checking the download")) + " — "
                   + qsTr("only updates signed by the CustomOS key are installed")
        case "installing":
            return qsTr("You can keep working. Your current system isn't changed until you restart.")
        case "confirming":
            return s.progressText || ""
        case "reboot-required":
            return s.pendingKind === "rollback"
                   ? qsTr("%1 %2 starts after a restart.").arg(view.osName).arg(s.pendingVersion || "")
                   : qsTr("%1 %2 is installed and starts after a restart.").arg(view.osName).arg(s.pendingVersion || "")
        case "error":
            return s.errorMessage || ""
        default:
            return s.lastCheck ? qsTr("Last checked %1").arg(view.when(s.lastCheck))
                               : qsTr("Check whether a newer version of %1 is available.").arg(view.osName)
        }
    }

    ColumnLayout {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Theme.spacingXL
        spacing: Theme.spacingL

        RowLayout {
            Layout.fillWidth: true
            spacing: 18

            Rectangle {
                Layout.alignment: Qt.AlignTop
                implicitWidth: 56
                implicitHeight: 56
                radius: 28
                color: Theme.alpha(card.tone, Theme.dark ? 0.2 : 0.12)
                Icon {
                    anchors.centerIn: parent
                    name: card.iconName
                    size: 28
                    color: card.tone
                    RotationAnimator on rotation {
                        running: card.st === "checking"
                        from: 0; to: 360; duration: 1400; loops: Animation.Infinite
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                Label {
                    Layout.fillWidth: true
                    text: card.title
                    font.pixelSize: Theme.fontTitle
                    font.weight: Font.DemiBold
                    wrapMode: Text.Wrap
                }
                Label {
                    Layout.fillWidth: true
                    visible: text.length > 0
                    text: card.subtitle
                    color: card.isError && !card.rolledBack ? Theme.text : Theme.textSecondary
                    wrapMode: Text.Wrap
                }
            }
        }

        // Download / installation progress
        ColumnLayout {
            Layout.fillWidth: true
            visible: card.hasProgress
            spacing: 6
            ProgressBar {
                Layout.fillWidth: true
                from: 0
                to: 100
                value: Math.max(0, Number(card.s.progress) || 0)
                indeterminate: (Number(card.s.progress) || -1) < 0 || card.st === "verifying" || card.st === "confirming"
            }
            RowLayout {
                Layout.fillWidth: true
                Label {
                    Layout.fillWidth: true
                    text: card.st === "installing" ? (card.s.progressText || "") : ""
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontCaption
                    elide: Text.ElideRight
                }
                Label {
                    visible: (Number(card.s.progress) || -1) >= 0 && card.st !== "verifying"
                    text: qsTr("%1%").arg(Number(card.s.progress) || 0)
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontCaption
                }
            }
        }

        // Technical details of an error
        ColumnLayout {
            Layout.fillWidth: true
            visible: card.isError && (card.s.errorDetail || "").length > 0
            spacing: 6
            Button {
                flat: true
                leftPadding: 0
                text: card.showDetail ? qsTr("Hide details") : qsTr("Show details")
                icon.name: card.showDetail ? "chevron-up" : "chevron-down"
                onClicked: card.showDetail = !card.showDetail
            }
            Rectangle {
                Layout.fillWidth: true
                visible: card.showDetail
                implicitHeight: detail.implicitHeight + 20
                radius: Theme.radius
                color: Theme.surfaceAlt
                Label {
                    id: detail
                    anchors.fill: parent
                    anchors.margins: 10
                    text: card.s.errorDetail || ""
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.fontCaption
                    wrapMode: Text.WrapAnywhere
                    color: Theme.textSecondary
                }
            }
        }

        // Actions
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingS
            visible: card.hasActions

            Item { Layout.fillWidth: true }
            Row {
                id: buttons
                spacing: Theme.spacingS
                layoutDirection: Qt.RightToLeft

                // (right to left: the primary action is the rightmost button)
                Button {
                    visible: card.st === "update-available"
                    highlighted: true
                    text: card.s.downloaded ? qsTr("Install now") : qsTr("Download and install")
                    icon.name: card.s.downloaded ? "update" : "downloads"
                    enabled: card.view.canAct && card.s.canInstall === true
                    onClicked: Updates.install()
                }
                Button {
                    visible: card.st === "update-available" && !card.s.downloaded
                    text: qsTr("Download only")
                    enabled: card.view.canAct && card.s.canInstall === true
                    onClicked: Updates.download()
                }
                Button {
                    visible: card.st === "reboot-required"
                    highlighted: true
                    text: qsTr("Restart now")
                    icon.name: "restart"
                    enabled: card.view.canAct
                    onClicked: card.restartRequested()
                }
                Button {
                    visible: card.st === "downloading"
                    text: qsTr("Cancel")
                    enabled: card.view.canAct
                    onClicked: Updates.cancel()
                }
                Button {
                    visible: card.isError && !card.rolledBack
                    highlighted: true
                    text: qsTr("Try again")
                    icon.name: "refresh"
                    enabled: card.view.canAct
                    onClicked: card.s.updateAvailable ? Updates.install() : Updates.checkForUpdates()
                }
                Button {
                    visible: card.isError
                    text: card.rolledBack ? qsTr("OK") : qsTr("Dismiss")
                    highlighted: card.rolledBack
                    enabled: card.view.canAct
                    onClicked: Updates.dismissError()
                }
                Button {
                    visible: card.st === "idle" || card.st === "up-to-date"
                    highlighted: card.st === "idle"
                    text: qsTr("Check for updates")
                    icon.name: "refresh"
                    enabled: card.view.canAct
                    onClicked: Updates.checkForUpdates()
                }
                Button {
                    visible: card.st === "update-available"
                    flat: true
                    text: qsTr("Check again")
                    enabled: card.view.canAct
                    onClicked: Updates.checkForUpdates()
                }
                Button {
                    visible: card.st === "unavailable"
                    text: qsTr("Try again")
                    icon.name: "refresh"
                    onClicked: Updates.refresh()
                }
            }
        }
    }
}
