import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Settings

SettingsPage {
    id: page
    title: qsTr("Your account")
    subtitle: qsTr("Your name on the desktop and your sign-in password.")

    readonly property var groups: SystemInfo.userGroups()
    readonly property bool admin: groups.indexOf("wheel") >= 0 || groups.indexOf("sudo") >= 0

    Connections {
        target: SystemActions
        function onFinished(action, ok, message) {
            if (action !== "password") return
            page.notify(message, !ok)
            if (ok) { currentPw.text = ""; newPw.text = ""; confirmPw.text = "" }
        }
    }

    SectionCard {
        Layout.fillWidth: true
        RowLayout {
            spacing: 18
            Rectangle {
                width: 72; height: 72; radius: 36
                color: Theme.accent
                Text {
                    anchors.centerIn: parent
                    text: (Settings.userDisplayName || Sys.userFullName()).charAt(0).toUpperCase()
                    color: Theme.onAccent
                    font.pixelSize: 32
                    font.weight: Font.DemiBold
                }
            }
            ColumnLayout {
                spacing: 2
                Label { text: Settings.userDisplayName || Sys.userFullName(); font.pixelSize: Theme.fontTitle; font.weight: Font.DemiBold }
                Label { text: page.admin ? qsTr("Administrator") : qsTr("Standard user"); color: Theme.textSecondary }
                Label { text: Sys.userName + " • " + Sys.homePath; color: Theme.textSecondary; font.pixelSize: Theme.fontCaption }
            }
        }
    }

    SectionCard {
        Layout.fillWidth: true
        SettingRow {
            title: qsTr("Display name")
            description: qsTr("Shown in Start and on the lock screen")
            iconName: "user"
            TextField {
                id: nameField
                Layout.preferredWidth: 260
                text: Settings.userDisplayName || Sys.userFullName()
                onEditingFinished: Settings.userDisplayName = text.trim() === Sys.userFullName() ? "" : text.trim()
            }
        }
        Divider {}
        SettingRow {
            title: qsTr("Groups")
            description: page.groups.join(", ")
            iconName: "users"
        }
    }

    SectionCard {
        Layout.fillWidth: true
        Heading { text: qsTr("Change password"); level: 3 }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Theme.textSecondary
            text: page.admin ? qsTr("Enter your current password, then the new one twice.")
                             : qsTr("Only administrators can change their password here. Use the passwd command in a terminal instead.")
        }
        GridLayout {
            columns: 2
            columnSpacing: 16
            rowSpacing: 10
            enabled: page.admin
            Label { text: qsTr("Current password") }
            TextField { id: currentPw; echoMode: TextInput.Password; Layout.preferredWidth: 260 }
            Label { text: qsTr("New password") }
            TextField { id: newPw; echoMode: TextInput.Password; Layout.preferredWidth: 260 }
            Label { text: qsTr("Confirm new password") }
            TextField { id: confirmPw; echoMode: TextInput.Password; Layout.preferredWidth: 260 }
        }
        Label {
            visible: newPw.text.length > 0 && confirmPw.text.length > 0 && newPw.text !== confirmPw.text
            text: qsTr("The new passwords don't match.")
            color: Theme.danger
        }
        Button {
            text: qsTr("Change password")
            highlighted: true
            enabled: page.admin && currentPw.text.length > 0 && newPw.text.length >= 4 && newPw.text === confirmPw.text
            onClicked: SystemActions.changePassword(currentPw.text, newPw.text)
        }
    }
}
