import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Meridian.Core
import Meridian.Greeter

// The sign-in screen.
Window {
    id: win
    visible: true
    visibility: Window.FullScreen
    width: 1280
    height: 800
    color: "#101615"
    title: qsTr("Sign in")

    property string selectedUser: Greeter.users.length === 1 ? Greeter.users[0].name : ""
    property string error: ""
    property date now: new Date()
    Timer { interval: 1000; running: true; repeat: true; onTriggered: win.now = new Date() }

    function selectUser(name) {
        selectedUser = name
        error = ""
        password.text = ""
        password.forceActiveFocus()
    }
    function submit() {
        if (!selectedUser.length) return
        error = ""
        Greeter.login(selectedUser, password.text)
    }

    Connections {
        target: Greeter
        function onFailed(message) { win.error = message; password.selectAll(); password.forceActiveFocus() }
    }

    Image {
        anchors.fill: parent
        source: Sys.fileUrl(Sys.dataDir + "/wallpapers/ridge-night.jpg")
        sourceSize: Qt.size(width, height)
        fillMode: Image.PreserveAspectCrop
    }
    Rectangle { anchors.fill: parent; color: Qt.rgba(0.03, 0.06, 0.06, 0.45) }

    // Clock
    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height * 0.10
        spacing: 2
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Qt.formatTime(win.now, "HH:mm")
            color: "#FFFFFF"
            font.pixelSize: 80
            font.weight: Font.DemiBold
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: win.now.toLocaleDateString(Qt.locale(), "dddd, d MMMM")
            color: "#E9EFED"
            font.pixelSize: 20
        }
    }

    // Users / password
    ColumnLayout {
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height * 0.40
        width: 340
        spacing: 16

        // Several accounts: pick one
        Flow {
            Layout.alignment: Qt.AlignHCenter
            visible: !win.selectedUser.length
            spacing: 18
            Repeater {
                model: Greeter.users
                delegate: Column {
                    required property var modelData
                    spacing: 10
                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 96; height: 96; radius: 48
                        color: userMouse.containsMouse ? "#2E9283" : "#1F8A7A"
                        border.width: 3
                        border.color: Qt.rgba(1, 1, 1, 0.25)
                        Text { anchors.centerIn: parent; text: modelData.fullName.charAt(0).toUpperCase(); color: "#FFFFFF"; font.pixelSize: 40; font.weight: Font.DemiBold }
                        MouseArea { id: userMouse; anchors.fill: parent; hoverEnabled: true; onClicked: win.selectUser(modelData.name) }
                    }
                    Text { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.fullName; color: "#FFFFFF"; font.pixelSize: 16 }
                }
            }
            Text {
                visible: Greeter.users.length === 0
                text: qsTr("No user accounts exist. Press Ctrl+Alt+F2 to sign in on a text console.")
                color: "#FFFFFF"
            }
        }

        // One account chosen: avatar, name, password
        ColumnLayout {
            visible: win.selectedUser.length > 0
            Layout.fillWidth: true
            spacing: 14
            readonly property var info: Greeter.users.find(u => u.name === win.selectedUser) || ({ fullName: win.selectedUser })
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: 104; height: 104; radius: 52
                color: "#1F8A7A"
                border.width: 3
                border.color: Qt.rgba(1, 1, 1, 0.25)
                Text { anchors.centerIn: parent; text: (parent.parent.info.fullName || "?").charAt(0).toUpperCase(); color: "#FFFFFF"; font.pixelSize: 44; font.weight: Font.DemiBold }
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: parent.info.fullName
                color: "#FFFFFF"
                font.pixelSize: 22
                font.weight: Font.DemiBold
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                TextField {
                    id: password
                    Layout.fillWidth: true
                    echoMode: TextInput.Password
                    placeholderText: qsTr("Password")
                    enabled: !Greeter.busy
                    focus: true
                    onAccepted: win.submit()
                }
                Button {
                    icon.name: "arrow-right"
                    highlighted: true
                    enabled: !Greeter.busy
                    onClicked: win.submit()
                }
            }
            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                visible: win.error.length > 0
                text: win.error
                wrapMode: Text.Wrap
                color: "#FFD6D3"
                font.pixelSize: 14
            }
            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                visible: Greeter.live && win.selectedUser === "live"
                text: qsTr("Live session — the password is “live”")
                color: "#C9D6D2"
                font.pixelSize: 13
            }
            Button {
                Layout.alignment: Qt.AlignHCenter
                visible: Greeter.users.length > 1
                flat: true
                text: qsTr("Other user")
                onClicked: win.selectedUser = ""
                palette.buttonText: "#FFFFFF"
            }
        }
    }

    // Bottom bar: computer name and power
    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 24
        Text { text: Greeter.hostName; color: "#C9D6D2"; font.pixelSize: 14; Layout.fillWidth: true }
        Rectangle {
            width: 44; height: 44; radius: 22
            color: rebootMouse.containsMouse ? Qt.rgba(1, 1, 1, 0.18) : Qt.rgba(1, 1, 1, 0.08)
            Icon { anchors.centerIn: parent; name: "restart"; size: 20; color: "#FFFFFF" }
            MouseArea { id: rebootMouse; anchors.fill: parent; hoverEnabled: true; onClicked: Greeter.reboot() }
        }
        Rectangle {
            width: 44; height: 44; radius: 22
            color: powerMouse.containsMouse ? Qt.rgba(1, 1, 1, 0.18) : Qt.rgba(1, 1, 1, 0.08)
            Icon { anchors.centerIn: parent; name: "power"; size: 20; color: "#FFFFFF" }
            MouseArea { id: powerMouse; anchors.fill: parent; hoverEnabled: true; onClicked: Greeter.powerOff() }
        }
    }

    Component.onCompleted: if (selectedUser.length) password.forceActiveFocus()
}
