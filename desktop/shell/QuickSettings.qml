import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Shell

// Quick settings: network, Wi-Fi, do-not-disturb, theme, volume and output
// device, with drill-in pages for Wi-Fi networks and sound outputs.
Flyout {
    id: qs
    property string page: "main"     // main | wifi | sound

    width: 380
    height: content.implicitHeight

    onOpenChanged: {
        Audio.active = open
        Network.active = open
        if (!open) page = "main"
    }

    ColumnLayout {
        id: content
        width: parent.width
        spacing: 0

        // ---------------- main page ----------------
        ColumnLayout {
            visible: qs.page === "main"
            Layout.fillWidth: true
            Layout.margins: 20
            spacing: 16

            GridLayout {
                Layout.fillWidth: true
                columns: 3
                columnSpacing: 10
                rowSpacing: 10

                QuickTile {
                    visible: Network.wifiHardware
                    label: qsTr("Wi-Fi")
                    sublabel: !Network.wifiEnabled ? qsTr("Off")
                              : Network.primaryType === "wifi" ? Network.primaryName : qsTr("Not connected")
                    iconName: Network.wifiEnabled ? "wifi-4" : "wifi-off"
                    checked: Network.wifiEnabled
                    hasDetails: true
                    onToggled: Network.setWifiEnabled(!Network.wifiEnabled)
                    onDetailsRequested: { qs.page = "wifi"; Network.scanWifi() }
                }
                QuickTile {
                    label: !Network.available ? qsTr("Network")
                           : Network.primaryType === "ethernet" ? qsTr("Ethernet")
                           : Network.primaryType === "wifi" ? qsTr("Online") : qsTr("Network")
                    sublabel: !Network.available ? qsTr("Unavailable")
                              : !Network.networkingEnabled ? qsTr("Disabled")
                              : Network.state === "connected" ? qsTr("Connected")
                              : Network.state === "connecting" ? qsTr("Connecting…") : qsTr("Disconnected")
                    iconName: Network.primaryType === "ethernet" ? "ethernet"
                              : Network.state === "connected" ? "globe" : "network-off"
                    checked: Network.available && Network.networkingEnabled
                    enabled: Network.available
                    onToggled: Network.setNetworkingEnabled(!Network.networkingEnabled)
                }
                QuickTile {
                    label: qsTr("Do not disturb")
                    sublabel: Settings.doNotDisturb ? qsTr("On") : qsTr("Off")
                    iconName: Settings.doNotDisturb ? "bell-off" : "bell"
                    checked: Settings.doNotDisturb
                    onToggled: Settings.doNotDisturb = !Settings.doNotDisturb
                }
                QuickTile {
                    label: qsTr("Dark mode")
                    sublabel: Settings.darkMode ? qsTr("On") : qsTr("Off")
                    iconName: "moon"
                    checked: Settings.darkMode
                    onToggled: Settings.darkMode = !Settings.darkMode
                }
                QuickTile {
                    label: qsTr("Desktops")
                    sublabel: Workspaces.available ? qsTr("%1 of %2").arg(Workspaces.current + 1).arg(Workspaces.count)
                                                   : qsTr("Task view")
                    iconName: "desktops"
                    checked: false
                    onToggled: ShellState.open("taskview")
                }
                QuickTile {
                    label: qsTr("Screenshot")
                    sublabel: qsTr("Whole screen")
                    iconName: "camera"
                    checked: false
                    onToggled: { qs.closeRequested(); shotTimer.start() }
                }
            }

            // Volume
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                ToolButton {
                    enabled: Audio.available
                    icon.name: !Audio.available || Audio.muted || Audio.volume === 0 ? "volume-mute"
                             : Audio.volume < 34 ? "volume-low" : Audio.volume < 67 ? "volume-medium" : "volume-high"
                    onClicked: Audio.toggleMute()
                    ToolTip.visible: hovered
                    ToolTip.text: Audio.muted ? qsTr("Unmute") : qsTr("Mute")
                }
                Slider {
                    id: volumeSlider
                    Layout.fillWidth: true
                    visible: Audio.available
                    from: 0; to: 100; stepSize: 1
                    value: Audio.volume
                    onMoved: Audio.setVolume(value)
                }
                Label {
                    visible: Audio.available
                    text: Audio.muted ? qsTr("Muted") : Audio.volume + "%"
                    color: Theme.textSecondary
                    Layout.preferredWidth: 46
                    horizontalAlignment: Text.AlignRight
                }
                Label {
                    visible: !Audio.available
                    Layout.fillWidth: true
                    text: qsTr("No audio service is running")
                    color: Theme.textSecondary
                }
                ToolButton {
                    visible: Audio.available
                    icon.name: "chevron-right"
                    onClicked: { qs.page = "sound"; Audio.refreshDevices() }
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Choose output device")
                }
            }
        }

        // ---------------- Wi-Fi page ----------------
        ColumnLayout {
            visible: qs.page === "wifi"
            Layout.fillWidth: true
            Layout.margins: 16
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                ToolButton { icon.name: "chevron-left"; onClicked: qs.page = "main" }
                Heading { text: qsTr("Wi-Fi"); level: 2; Layout.fillWidth: true }
                BusyIndicator { running: Network.scanning || Network.busy; implicitWidth: 22; implicitHeight: 22 }
                Switch {
                    checked: Network.wifiEnabled
                    onToggled: Network.setWifiEnabled(checked)
                }
            }
            ListView {
                id: apList
                Layout.fillWidth: true
                Layout.preferredHeight: 300
                clip: true
                model: Network.wifiEnabled ? Network.accessPoints : []
                property string expanded: ""
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {}
                delegate: Column {
                    required property var modelData
                    width: ListView.view.width
                    ItemDelegate {
                        width: parent.width
                        text: modelData.ssid
                        subtitle: modelData.inUse ? qsTr("Connected") : modelData.secure ? qsTr("Secured") : qsTr("Open")
                        icon.name: modelData.secure ? "wifi-lock"
                                 : modelData.signal > 75 ? "wifi-4" : modelData.signal > 50 ? "wifi-3" : modelData.signal > 25 ? "wifi-2" : "wifi-1"
                        highlighted: modelData.inUse
                        onClicked: apList.expanded = apList.expanded === modelData.ssid ? "" : modelData.ssid
                    }
                    RowLayout {
                        visible: apList.expanded === modelData.ssid
                        width: parent.width
                        spacing: 8
                        TextField {
                            id: pw
                            visible: modelData.secure && !modelData.inUse
                            Layout.fillWidth: true
                            echoMode: TextInput.Password
                            placeholderText: qsTr("Password")
                            onAccepted: Network.connectWifi(modelData.ssid, text)
                        }
                        Item { Layout.fillWidth: true; visible: !pw.visible }
                        Button {
                            text: modelData.inUse ? qsTr("Disconnect") : qsTr("Connect")
                            highlighted: !modelData.inUse
                            enabled: !Network.busy
                            onClicked: {
                                if (modelData.inUse) {
                                    const d = Network.devices.find(x => x.type === "wifi")
                                    if (d) Network.disconnectDevice(d.device)
                                } else {
                                    Network.connectWifi(modelData.ssid, pw.text)
                                }
                            }
                        }
                    }
                }
                Label {
                    anchors.centerIn: parent
                    width: parent.width - 40
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    visible: apList.count === 0
                    text: !Network.wifiEnabled ? qsTr("Wi-Fi is turned off.")
                          : Network.scanning ? qsTr("Looking for networks…") : qsTr("No networks found.")
                    color: Theme.textSecondary
                }
            }
            Label {
                id: wifiStatus
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                visible: text.length > 0
                color: Theme.textSecondary
                font.pixelSize: Theme.fontCaption
                Connections {
                    target: Network
                    function onActionFinished(ok, message) { wifiStatus.text = message }
                }
            }
        }

        // ---------------- sound output page ----------------
        ColumnLayout {
            visible: qs.page === "sound"
            Layout.fillWidth: true
            Layout.margins: 16
            spacing: 10
            RowLayout {
                Layout.fillWidth: true
                ToolButton { icon.name: "chevron-left"; onClicked: qs.page = "main" }
                Heading { text: qsTr("Sound output"); level: 2; Layout.fillWidth: true }
            }
            Repeater {
                model: Audio.sinks
                delegate: RadioButton {
                    required property var modelData
                    Layout.fillWidth: true
                    text: modelData.name
                    checked: modelData.isDefault
                    onClicked: Audio.setDefaultSink(modelData.id)
                }
            }
            Label {
                visible: Audio.sinks.length === 0
                text: qsTr("No output devices found.")
                color: Theme.textSecondary
            }
        }

        // ---------------- footer ----------------
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: Theme.dark ? Qt.rgba(1, 1, 1, 0.03) : Qt.rgba(0, 0, 0, 0.025)
            radius: Theme.radiusXL
            Rectangle { width: parent.width; height: 1; color: Theme.divider }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 12
                Label {
                    Layout.fillWidth: true
                    text: Sys.isLive ? qsTr("Live session") : Sys.hostName()
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontCaption
                    elide: Text.ElideRight
                }
                ToolButton {
                    icon.name: "lock"
                    onClicked: { qs.closeRequested(); Session.lock() }
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Lock")
                }
                ToolButton {
                    icon.name: "settings"
                    onClicked: { qs.closeRequested(); Apps.launch("meridian-settings.desktop") }
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("All settings")
                }
                ToolButton {
                    id: qsPower
                    icon.name: "power"
                    onClicked: powerMenu.popup(qsPower, 0, -powerMenu.implicitHeight - 4)
                }
            }
        }
    }

    Timer {
        id: shotTimer
        interval: Theme.duration + 120
        onTriggered: Session.screenshot()
    }

    Menu {
        id: powerMenu
        MenuItem { text: qsTr("Sign out"); icon.name: "logout"; onTriggered: Session.logout() }
        MenuItem { text: qsTr("Restart"); icon.name: "restart"; onTriggered: Session.reboot() }
        MenuItem { text: qsTr("Shut down"); icon.name: "power"; onTriggered: Session.powerOff() }
    }
}
