import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Settings

SettingsPage {
    id: page
    title: qsTr("Network")
    subtitle: qsTr("Connections are managed by NetworkManager.")

    property var details: ({})
    Component.onCompleted: { Network.active = true; Network.refresh(); refreshDetails() }
    Component.onDestruction: Network.active = false
    function refreshDetails() {
        for (const d of Network.devices) Network.requestDetails(d.device)
    }
    Connections {
        target: Network
        function onDetailsReady(device, d) { const c = Object.assign({}, page.details); c[device] = d; page.details = c }
        function onActionFinished(ok, message) { page.notify(message, !ok); page.refreshDetails() }
        function onChanged() { detailsTimer.restart() }
    }
    Timer { id: detailsTimer; interval: 800; onTriggered: page.refreshDetails() }

    SectionCard {
        Layout.fillWidth: true
        visible: !Network.available
        RowLayout {
            Icon { name: "network-off"; size: 20; color: Theme.textSecondary }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                color: Theme.textSecondary
                text: qsTr("NetworkManager is not running, so network settings are unavailable.")
            }
        }
    }

    SectionCard {
        Layout.fillWidth: true
        visible: Network.available
        SettingRow {
            title: qsTr("Networking")
            description: Network.state === "connected" ? qsTr("Connected to the internet or local network")
                         : Network.state === "connecting" ? qsTr("Connecting…") : qsTr("Not connected")
            iconName: Network.state === "connected" ? "globe" : "network-off"
            Switch { checked: Network.networkingEnabled; onToggled: Network.setNetworkingEnabled(checked) }
        }
        SettingRow {
            visible: Network.wifiHardware
            title: qsTr("Wi-Fi")
            description: Network.wifiEnabled ? qsTr("Choose a network below") : qsTr("Off")
            iconName: "wifi-4"
            Switch { checked: Network.wifiEnabled; onToggled: Network.setWifiEnabled(checked) }
        }
    }

    // Devices
    Repeater {
        model: Network.available ? Network.devices : []
        delegate: SectionCard {
            id: devCard
            required property var modelData
            readonly property var d: page.details[modelData.device] || ({})
            readonly property bool connected: modelData.state.startsWith("connected")
            Layout.fillWidth: true
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Icon { name: devCard.modelData.type === "wifi" ? "wifi-4" : "ethernet"; size: 22 }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    Label {
                        text: (devCard.modelData.type === "wifi" ? qsTr("Wi-Fi") : qsTr("Ethernet")) + " (" + devCard.modelData.device + ")"
                        font.weight: Font.DemiBold
                    }
                    Label {
                        text: devCard.connected ? qsTr("Connected • %1").arg(devCard.modelData.connection)
                                                : devCard.modelData.state.replace(/\(.*\)/, "").trim()
                        color: devCard.connected ? Theme.success : Theme.textSecondary
                        font.pixelSize: Theme.fontCaption
                    }
                }
                Button {
                    text: devCard.connected ? qsTr("Disconnect") : qsTr("Connect")
                    enabled: !Network.busy && devCard.modelData.state !== "unavailable"
                    onClicked: devCard.connected ? Network.disconnectDevice(devCard.modelData.device)
                                                 : Network.connectDevice(devCard.modelData.device)
                }
            }
            GridLayout {
                visible: devCard.connected
                columns: 2
                columnSpacing: 24
                rowSpacing: 6
                Layout.fillWidth: true
                Label { text: qsTr("IPv4 address"); color: Theme.textSecondary }
                Label { text: devCard.d.ipv4 || "—"; font.family: Theme.monoFamily }
                Label { text: qsTr("Gateway"); color: Theme.textSecondary }
                Label { text: devCard.d.gateway || "—"; font.family: Theme.monoFamily }
                Label { text: qsTr("DNS servers"); color: Theme.textSecondary }
                Label { text: devCard.d.dns || "—"; font.family: Theme.monoFamily }
                Label { text: qsTr("IPv6 address"); color: Theme.textSecondary; visible: (devCard.d.ipv6 || "").length > 0 }
                Label { text: devCard.d.ipv6 || ""; font.family: Theme.monoFamily; visible: text.length > 0; Layout.fillWidth: true; wrapMode: Text.WrapAnywhere }
                Label { text: qsTr("Hardware address"); color: Theme.textSecondary }
                Label { text: devCard.d.mac || "—"; font.family: Theme.monoFamily }
            }
        }
    }

    // Wi-Fi networks
    SectionCard {
        Layout.fillWidth: true
        visible: Network.available && Network.wifiHardware && Network.wifiEnabled
        RowLayout {
            Layout.fillWidth: true
            Heading { text: qsTr("Wi-Fi networks"); level: 3; Layout.fillWidth: true }
            BusyIndicator { running: Network.scanning; implicitWidth: 22; implicitHeight: 22 }
            Button { text: qsTr("Scan"); onClicked: Network.scanWifi() }
        }
        Repeater {
            model: Network.accessPoints
            delegate: RowLayout {
                id: apRow
                required property var modelData
                Layout.fillWidth: true
                spacing: 12
                Icon { name: modelData.secure ? "wifi-lock" : (modelData.signal > 66 ? "wifi-4" : modelData.signal > 33 ? "wifi-3" : "wifi-2"); size: 18 }
                Label { text: modelData.ssid; Layout.fillWidth: true; font.weight: modelData.inUse ? Font.DemiBold : Font.Normal }
                Label { text: modelData.signal + "%"; color: Theme.textSecondary }
                TextField {
                    id: pw
                    visible: modelData.secure && !modelData.inUse
                    Layout.preferredWidth: 180
                    echoMode: TextInput.Password
                    placeholderText: qsTr("Password")
                }
                Button {
                    text: modelData.inUse ? qsTr("Connected") : qsTr("Connect")
                    enabled: !modelData.inUse && !Network.busy
                    highlighted: !modelData.inUse
                    onClicked: Network.connectWifi(modelData.ssid, pw.text)
                }
            }
        }
        Label {
            visible: Network.accessPoints.length === 0
            text: Network.scanning ? qsTr("Looking for networks…") : qsTr("No networks found. Press Scan to look again.")
            color: Theme.textSecondary
        }
    }
}
