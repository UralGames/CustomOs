import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Installer

ApplicationWindow {
    id: win
    width: 860
    height: 600
    minimumWidth: 720
    minimumHeight: 520
    visible: true
    title: qsTr("Install %1").arg(osName)

    readonly property string osName: SystemInfo.osName || "CustomOS"
    property int step: 0          // 0 welcome, 1 disk, 2 account, 3 confirm, 4 installing, 5 done
    property string disk: ""
    property var diskInfo: Installer.disks.find(d => d.path === disk) || ({})
    property bool success: false
    property string resultMessage: ""

    readonly property var steps: [qsTr("Welcome"), qsTr("Disk"), qsTr("Account"), qsTr("Confirm"), qsTr("Install"), qsTr("Done")]

    onClosing: (close) => { if (Installer.running) close.accepted = false }

    Connections {
        target: Installer
        function onFinished(ok, message) { win.success = ok; win.resultMessage = message; win.step = 5 }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Step list
        Rectangle {
            Layout.preferredWidth: 230
            Layout.fillHeight: true
            color: Theme.sidebar
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 6
                RowLayout {
                    spacing: 12
                    Layout.bottomMargin: 20
                    AppIcon { name: "customos-logo"; size: 40 }
                    Label { text: win.osName; font.pixelSize: Theme.fontSubtitle; font.weight: Font.DemiBold }
                }
                Repeater {
                    model: win.steps
                    delegate: RowLayout {
                        required property string modelData
                        required property int index
                        spacing: 10
                        Rectangle {
                            width: 24; height: 24; radius: 12
                            color: index < win.step ? Theme.accent : index === win.step ? Theme.accentSoft : "transparent"
                            border.width: index <= win.step ? 0 : 1
                            border.color: Theme.borderStrong
                            Icon { anchors.centerIn: parent; visible: index < win.step; name: "check"; size: 14; color: Theme.onAccent }
                            Text { anchors.centerIn: parent; visible: index >= win.step; text: index + 1; font.pixelSize: 11
                                   color: index === win.step ? Theme.accent : Theme.textTertiary }
                        }
                        Label {
                            text: modelData
                            font.weight: index === win.step ? Font.DemiBold : Font.Normal
                            color: index <= win.step ? Theme.text : Theme.textTertiary
                        }
                    }
                }
                Item { Layout.fillHeight: true }
            }
        }

        // Pages
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 32
                currentIndex: win.step

                // 0 welcome
                ColumnLayout {
                    spacing: 14
                    Heading { text: qsTr("Install %1 on this computer").arg(win.osName) }
                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        color: Theme.textSecondary
                        text: qsTr("The installer copies the system you are using now onto a disk, creates your user account "
                                   + "and sets up the boot loader. It takes a few minutes.")
                    }
                    SectionCard {
                        Layout.fillWidth: true
                        Heading { text: qsTr("You will need"); level: 3 }
                        Label { text: qsTr("• A disk of at least 8 GB that may be completely erased"); wrapMode: Text.Wrap; Layout.fillWidth: true }
                        Label { text: qsTr("• A name and password for your account"); wrapMode: Text.Wrap; Layout.fillWidth: true }
                    }
                    Item { Layout.fillHeight: true }
                }

                // 1 disk
                ColumnLayout {
                    spacing: 14
                    Heading { text: qsTr("Where should %1 be installed?").arg(win.osName) }
                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        color: Theme.textSecondary
                        text: qsTr("The whole disk you choose will be erased. Other disks are not touched.")
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Item { Layout.fillWidth: true }
                        Button { text: qsTr("Refresh"); icon.name: "refresh"; onClicked: Installer.refreshDisks() }
                    }
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 8
                        model: Installer.disks
                        delegate: Rectangle {
                            required property var modelData
                            width: ListView.view.width
                            height: 68
                            radius: Theme.radiusLarge
                            color: win.disk === modelData.path ? Theme.accentSoft : diskMouse.containsMouse ? Theme.hover : Theme.surface
                            border.width: win.disk === modelData.path ? 2 : 1
                            border.color: win.disk === modelData.path ? Theme.accent : Theme.border
                            opacity: modelData.bigEnough ? 1 : 0.5
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 14
                                spacing: 14
                                Icon { name: "harddisk"; size: 26 }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    Label { text: modelData.model + " — " + modelData.sizeText; font.weight: Font.DemiBold }
                                    Label {
                                        text: modelData.path + (modelData.transport ? " • " + modelData.transport.toUpperCase() : "")
                                              + (modelData.removable ? qsTr(" • removable") : "")
                                              + (modelData.bigEnough ? "" : qsTr(" • too small"))
                                        color: Theme.textSecondary
                                        font.pixelSize: Theme.fontCaption
                                    }
                                }
                                RadioButton { checked: win.disk === modelData.path; enabled: modelData.bigEnough; onClicked: win.disk = modelData.path }
                            }
                            MouseArea { id: diskMouse; anchors.fill: parent; hoverEnabled: true; enabled: modelData.bigEnough; onClicked: win.disk = modelData.path }
                        }
                        Label {
                            anchors.centerIn: parent
                            visible: Installer.disks.length === 0
                            width: parent.width - 40
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.Wrap
                            text: qsTr("No disks were found. In a virtual machine, add a virtual hard disk (25 GB is plenty) and press Refresh.")
                            color: Theme.textSecondary
                        }
                    }
                }

                // 2 account
                ColumnLayout {
                    spacing: 14
                    Heading { text: qsTr("Create your account") }
                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        color: Theme.textSecondary
                        text: qsTr("This account will be an administrator (it can use sudo).")
                    }
                    GridLayout {
                        columns: 2
                        columnSpacing: 16
                        rowSpacing: 12
                        Label { text: qsTr("Your name") }
                        TextField {
                            id: fullName
                            Layout.preferredWidth: 320
                            placeholderText: qsTr("e.g. Ada Lovelace")
                            onTextEdited: if (!userName.edited) userName.text = Installer.suggestUserName(text)
                        }
                        Label { text: qsTr("User name") }
                        TextField {
                            id: userName
                            property bool edited: false
                            Layout.preferredWidth: 320
                            placeholderText: qsTr("lower-case letters and digits")
                            validator: RegularExpressionValidator { regularExpression: /[a-z_][a-z0-9_-]{0,31}/ }
                            onTextEdited: edited = true
                        }
                        Label { text: qsTr("Computer name") }
                        TextField {
                            id: hostName
                            Layout.preferredWidth: 320
                            text: "customos"
                            validator: RegularExpressionValidator { regularExpression: /[A-Za-z0-9][A-Za-z0-9-]{0,62}/ }
                        }
                        Label { text: qsTr("Password") }
                        TextField { id: pass1; Layout.preferredWidth: 320; echoMode: TextInput.Password }
                        Label { text: qsTr("Confirm password") }
                        TextField { id: pass2; Layout.preferredWidth: 320; echoMode: TextInput.Password }
                    }
                    Label {
                        visible: pass2.text.length > 0 && pass1.text !== pass2.text
                        text: qsTr("The passwords don't match.")
                        color: Theme.danger
                    }
                    Label {
                        visible: userName.text === "root" || userName.text === "live"
                        text: qsTr("Choose a different user name.")
                        color: Theme.danger
                    }
                    Item { Layout.fillHeight: true }
                }

                // 3 confirm
                ColumnLayout {
                    spacing: 14
                    Heading { text: qsTr("Ready to install") }
                    SectionCard {
                        Layout.fillWidth: true
                        GridLayout {
                            columns: 2
                            columnSpacing: 24
                            rowSpacing: 8
                            Label { text: qsTr("Disk"); color: Theme.textSecondary }
                            Label { text: (win.diskInfo.model || "") + " — " + (win.diskInfo.sizeText || "") + " (" + win.disk + ")" }
                            Label { text: qsTr("Account"); color: Theme.textSecondary }
                            Label { text: fullName.text + " (" + userName.text + ")" }
                            Label { text: qsTr("Computer name"); color: Theme.textSecondary }
                            Label { text: hostName.text }
                            Label { text: qsTr("Boot loader"); color: Theme.textSecondary }
                            Label {
                                text: Installer.uefi ? qsTr("GRUB (UEFI, with legacy BIOS fallback)")
                                                     : qsTr("GRUB (legacy BIOS, with UEFI fallback)")
                            }
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: warnCol.implicitHeight + 28
                        radius: Theme.radiusLarge
                        color: Theme.dangerBackground
                        ColumnLayout {
                            id: warnCol
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 10
                            RowLayout {
                                spacing: 10
                                Icon { name: "warning"; size: 20; color: Theme.danger }
                                Label {
                                    Layout.fillWidth: true
                                    wrapMode: Text.Wrap
                                    font.weight: Font.DemiBold
                                    text: qsTr("Everything on %1 (%2) will be permanently erased.").arg(win.disk).arg(win.diskInfo.sizeText || "")
                                }
                            }
                            CheckBox {
                                id: understand
                                text: qsTr("I understand that all data on this disk will be lost")
                            }
                        }
                    }
                    Item { Layout.fillHeight: true }
                }

                // 4 installing
                ColumnLayout {
                    spacing: 14
                    Heading { text: qsTr("Installing %1").arg(win.osName) }
                    Label { text: Installer.status; color: Theme.textSecondary }
                    ProgressBar { Layout.fillWidth: true; from: 0; to: 100; value: Installer.progress }
                    Label { text: Installer.progress + "%"; color: Theme.textSecondary }
                    Item { Layout.fillHeight: true }
                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        color: Theme.textTertiary
                        text: qsTr("Please don't turn off the computer.")
                    }
                }

                // 5 done
                ColumnLayout {
                    spacing: 14
                    RowLayout {
                        spacing: 12
                        Icon { name: win.success ? "check" : "warning"; size: 28; color: win.success ? Theme.success : Theme.danger }
                        Heading { text: win.success ? qsTr("Installation complete") : qsTr("Installation failed") }
                    }
                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: win.success ? qsTr("Remove the installation medium (in a virtual machine: eject the ISO from the optical drive), "
                                                 + "then restart to start %1 from the disk. Sign in as %2.").arg(win.osName).arg(userName.text)
                                          : win.resultMessage
                        color: win.success ? Theme.text : Theme.danger
                    }
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: Installer.log.length > 0 && !win.success
                        TextArea { readOnly: true; text: Installer.log; font.family: Theme.monoFamily; font.pixelSize: 12 }
                    }
                    Item { Layout.fillHeight: true; visible: win.success || Installer.log.length === 0 }
                }
            }

            // Navigation
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 64
                color: Theme.window
                Rectangle { width: parent.width; height: 1; color: Theme.divider }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 32
                    anchors.rightMargin: 24
                    spacing: 10
                    Item { Layout.fillWidth: true }
                    Button {
                        text: qsTr("Back")
                        visible: win.step > 0 && win.step < 4
                        onClicked: win.step -= 1
                    }
                    Button {
                        visible: win.step < 3
                        text: qsTr("Next")
                        highlighted: true
                        enabled: win.step === 0 ? true
                               : win.step === 1 ? win.disk.length > 0
                               : fullName.text.trim().length > 0 && userName.acceptableInput && userName.text !== "root"
                                 && userName.text !== "live" && hostName.acceptableInput && pass1.text.length >= 4 && pass1.text === pass2.text
                        onClicked: win.step += 1
                    }
                    Button {
                        visible: win.step === 3
                        text: qsTr("Erase disk and install")
                        highlighted: true
                        enabled: understand.checked
                        onClicked: {
                            win.step = 4
                            Installer.install(win.disk, win.disk, userName.text, fullName.text.trim(), hostName.text, pass1.text)
                        }
                    }
                    Button {
                        visible: win.step === 5
                        text: win.success ? qsTr("Close") : qsTr("Back to start")
                        onClicked: win.success ? Qt.quit() : (win.step = 0)
                    }
                    Button {
                        visible: win.step === 5 && win.success
                        text: qsTr("Restart now")
                        highlighted: true
                        onClicked: Installer.reboot()
                    }
                }
            }
        }
    }
}
