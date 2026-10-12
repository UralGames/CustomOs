import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Settings

SettingsPage {
    id: page
    title: qsTr("System")
    subtitle: qsTr("About this computer and the operating system.")

    property var disk: SystemInfo.diskUsage("/")
    property string uptime: Sys.formatDuration(SystemInfo.uptimeSeconds())
    Timer { interval: 30000; running: true; repeat: true; onTriggered: { page.uptime = Sys.formatDuration(SystemInfo.uptimeSeconds()); page.disk = SystemInfo.diskUsage("/") } }

    // Header card with the OS identity
    Rectangle {
        Layout.fillWidth: true
        implicitHeight: 120
        radius: Theme.radiusLarge
        color: Theme.surface
        border.color: Theme.border
        RowLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 18
            AppIcon { name: "customos-logo"; size: 72 }
            ColumnLayout {
                spacing: 4
                Label { text: (SystemInfo.osName || "CustomOS") + " " + SystemInfo.osVersion; font.pixelSize: Theme.fontTitle; font.weight: Font.DemiBold }
                Label { text: qsTr("Meridian desktop %1 • Qt %2").arg(SystemInfo.desktopVersion).arg(SystemInfo.qtVersion); color: Theme.textSecondary }
                Label { text: Sys.isLive ? qsTr("Running from the live image") : qsTr("Installed on this computer"); color: Theme.textSecondary }
            }
        }
    }

    SectionCard {
        Layout.fillWidth: true
        Heading { text: qsTr("Computer name"); level: 3 }
        RowLayout {
            Layout.fillWidth: true
            TextField {
                id: hostField
                Layout.fillWidth: true
                text: Sys.hostName()
                onAccepted: SystemActions.setHostname(text.trim())
            }
            Button {
                text: qsTr("Rename")
                enabled: hostField.text.trim() !== Sys.hostName()
                onClicked: SystemActions.setHostname(hostField.text.trim())
            }
        }
    }

    SectionCard {
        Layout.fillWidth: true
        Heading { text: qsTr("Device specifications"); level: 3 }
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 24
            rowSpacing: 10
            Repeater {
                model: [
                    [qsTr("Processor"), (SystemInfo.cpuModel || qsTr("Unknown")) + qsTr(" (%1 threads)").arg(SystemInfo.cpuCount)],
                    [qsTr("Memory"), Sys.formatBytes(SystemInfo.memoryTotal)],
                    [qsTr("Storage"), qsTr("%1 used of %2 (%3)").arg(Sys.formatBytes(page.disk.total - page.disk.free)).arg(Sys.formatBytes(page.disk.total)).arg(page.disk.fsType)],
                    [qsTr("Graphics"), (SystemInfo.graphicsDriver || qsTr("none")) + " • "
                                       + (Sys.env("MERIDIAN_RENDERER") === "software" || SystemInfo.softwareRendering
                                          ? qsTr("software rendering") : qsTr("hardware rendering"))],
                    [qsTr("Machine"), SystemInfo.machine || qsTr("Unknown")],
                    [qsTr("Architecture"), SystemInfo.architecture],
                    [qsTr("Kernel"), SystemInfo.kernel],
                    [qsTr("Up time"), page.uptime]
                ]
                delegate: RowLayout {
                    required property var modelData
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    spacing: 24
                    Label { text: modelData[0]; color: Theme.textSecondary; Layout.preferredWidth: 140 }
                    Label { text: modelData[1]; Layout.fillWidth: true; wrapMode: Text.Wrap }
                }
            }
        }
    }

    SectionCard {
        Layout.fillWidth: true
        Heading { text: qsTr("Open source"); level: 3 }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Theme.textSecondary
            text: qsTr("This system is built from source and runs the Linux kernel, the GNU C library, systemd, "
                       + "the labwc Wayland compositor, Qt and Mesa, all free and open-source software. The build "
                       + "produces a complete list of components with their licences (legal-info).")
        }
    }

    Connections {
        target: SystemActions
        function onFinished(action, ok, message) { if (action === "hostname") page.notify(message, !ok) }
    }
}
