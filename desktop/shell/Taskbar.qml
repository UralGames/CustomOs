import QtQuick
import QtQuick.Controls
import Meridian.Core
import Meridian.Shell

// Taskbar content: start, task view, pinned + running apps (centred or
// left-aligned), then the tray (quick settings, clock and notifications).
Rectangle {
    id: bar
    property var items: []

    color: Theme.panel
    Behavior on color { ColorAnimation { duration: Theme.duration } }

    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: Theme.border
    }
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        onPressed: ShellState.closePanel()
    }

    // Group windows by application; pinned apps keep their place first.
    function rebuild() {
        const list = []
        const pinned = Settings.pinnedApps || []
        for (const id of pinned)
            if (Apps.exists(id)) list.push({ key: id, appId: id, pinned: true, windows: [] })
        for (const w of Toplevels.windows()) {
            const key = w.desktopId.length ? w.desktopId : "window:" + (w.appId || w.title)
            let entry = list.find(e => e.key === key)
            if (!entry) {
                entry = { key: key, appId: w.desktopId, pinned: false, windows: [] }
                list.push(entry)
            }
            entry.windows.push(w)
        }
        items = list
    }
    function activateIndex(i) {
        if (i >= 0 && i < appRepeater.count)
            appRepeater.itemAt(i).trigger()
    }

    Connections {
        target: Toplevels
        function onRevisionChanged() { rebuildTimer.restart() }
    }
    Connections {
        target: Settings
        function onSettingChanged(key) { if (key === "pinnedApps") bar.rebuild() }
    }
    Connections {
        target: Apps
        function onCountChanged() { bar.rebuild() }
    }
    // Coalesce bursts of window events into one rebuild.
    Timer { id: rebuildTimer; interval: 30; onTriggered: bar.rebuild() }
    Component.onCompleted: rebuild()

    Row {
        id: centre
        anchors.verticalCenter: parent.verticalCenter
        x: Settings.taskbarAlignment === "left" ? 8 : Math.round((bar.width - width) / 2)
        spacing: 2
        Behavior on x { NumberAnimation { duration: Theme.duration; easing.type: Theme.easing } }

        TaskbarIconButton {
            iconSource: "image://icon/app/customos-logo"
            highlighted: ShellState.panel === "launcher"
            tooltip: qsTr("Start")
            onClicked: ShellState.toggle("launcher")
        }
        TaskbarIconButton {
            symbolic: "taskview"
            highlighted: ShellState.panel === "taskview"
            tooltip: qsTr("Task view")
            badge: Workspaces.count > 1 && Workspaces.current >= 0 ? String(Workspaces.current + 1) : ""
            onClicked: ShellState.toggle("taskview")
        }
        Repeater {
            id: appRepeater
            model: bar.items
            delegate: TaskbarButton {
                required property var modelData
                entry: modelData
            }
        }
    }

    Row {
        id: tray
        anchors.right: parent.right
        anchors.rightMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2

        TrayButton {
            highlighted: ShellState.panel === "quick"
            tooltip: (Network.primaryType === "none" ? qsTr("Not connected") : Network.primaryName)
                     + " • " + (Audio.available ? (Audio.muted ? qsTr("Muted") : qsTr("Volume %1%").arg(Audio.volume)) : qsTr("No audio"))
            onClicked: ShellState.toggle("quick")
            contentItem: Row {
                spacing: 10
                Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    size: 16
                    name: !Network.available ? "network-off"
                        : Network.primaryType === "wifi" ? (Network.wifiSignal > 75 ? "wifi-4" : Network.wifiSignal > 50 ? "wifi-3" : Network.wifiSignal > 25 ? "wifi-2" : "wifi-1")
                        : Network.primaryType === "ethernet" ? "ethernet"
                        : (Network.wifiHardware && Network.wifiEnabled ? "wifi-off" : "network-off")
                }
                Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    size: 16
                    name: !Audio.available || Audio.muted || Audio.volume === 0 ? "volume-mute"
                        : Audio.volume < 34 ? "volume-low" : Audio.volume < 67 ? "volume-medium" : "volume-high"
                }
            }
        }

        TrayButton {
            highlighted: ShellState.panel === "calendar"
            tooltip: clock.now.toLocaleDateString(Qt.locale(), Locale.LongFormat)
            onClicked: ShellState.toggle("calendar")
            contentItem: Row {
                spacing: 10
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        anchors.right: parent.right
                        text: Qt.formatTime(clock.now, Settings.clock24h
                                            ? (Settings.showSeconds ? "HH:mm:ss" : "HH:mm")
                                            : (Settings.showSeconds ? "h:mm:ss AP" : "h:mm AP"))
                        font.pixelSize: 12
                        font.weight: Font.Medium
                        color: Theme.text
                    }
                    Text {
                        anchors.right: parent.right
                        text: Qt.formatDate(clock.now, Qt.locale().dateFormat(Locale.ShortFormat))
                        font.pixelSize: 12
                        color: Theme.text
                    }
                }
                Item {
                    width: Notifications.count > 0 || Settings.doNotDisturb ? 18 : 0
                    height: 18
                    anchors.verticalCenter: parent.verticalCenter
                    visible: width > 0
                    Icon {
                        anchors.centerIn: parent
                        visible: Settings.doNotDisturb
                        name: "bell-off"
                        size: 15
                    }
                    Rectangle {
                        anchors.centerIn: parent
                        visible: !Settings.doNotDisturb && Notifications.count > 0
                        width: Math.max(18, countText.implicitWidth + 8)
                        height: 18
                        radius: 9
                        color: Theme.accent
                        Text {
                            id: countText
                            anchors.centerIn: parent
                            text: Notifications.count > 9 ? "9+" : Notifications.count
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            color: Theme.onAccent
                        }
                    }
                }
            }
        }

        // "Show desktop" sliver at the far edge
        Item {
            width: 12
            height: bar.height
            Rectangle {
                anchors.centerIn: parent
                width: 1
                height: 18
                color: Theme.divider
            }
            MouseArea {
                id: sliver
                anchors.fill: parent
                hoverEnabled: true
                onClicked: { ShellState.closePanel(); Toplevels.showDesktop() }
                onContainsMouseChanged: containsMouse ? ShellState.showTooltip(qsTr("Show desktop"), bar.width - 60)
                                                      : ShellState.hideTooltip()
            }
        }
    }

    QtObject {
        id: clock
        property date now: new Date()
    }
    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: clock.now = new Date()
    }
}
