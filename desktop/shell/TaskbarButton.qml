import QtQuick
import Meridian.Core
import Meridian.Shell

// One application on the taskbar: pinned and/or running windows.
Item {
    id: btn
    property var entry          // { key, appId, pinned, windows: [toplevel maps] }

    readonly property var windows: entry ? entry.windows : []
    readonly property bool running: windows.length > 0
    readonly property bool isActive: windows.some(w => w.activated)
    readonly property var info: entry && entry.appId.length ? Apps.get(entry.appId) : null
    readonly property string iconName: info && info.iconName ? info.iconName : (running ? windows[0].iconName : "meridian-app")
    readonly property string label: info && info.name ? info.name : (running ? windows[0].name : "")
    property bool launching: false

    width: 44
    height: 44

    function trigger() {
        ShellState.closePanel()
        if (!running) {
            if (entry.appId.length) {
                launching = true
                launchTimer.restart()
                Apps.launch(entry.appId)
            }
            return
        }
        if (windows.length === 1) {
            Toplevels.toggle(windows[0].id)
            return
        }
        // Several windows: focus the next one in turn.
        const i = windows.findIndex(w => w.activated)
        Toplevels.activate(windows[(i + 1) % windows.length].id)
    }

    function showMenu() {
        const items = []
        if (entry.appId.length)
            items.push({ text: label, icon: "open", trigger: () => Apps.launch(entry.appId) })
        if (entry.appId.length)
            items.push({
                text: entry.pinned ? qsTr("Unpin from taskbar") : qsTr("Pin to taskbar"),
                icon: entry.pinned ? "unpin" : "pin",
                trigger: () => {
                    const list = (Settings.pinnedApps || []).slice()
                    const i = list.indexOf(entry.appId)
                    if (i >= 0) list.splice(i, 1)
                    else list.push(entry.appId)
                    Settings.pinnedApps = list
                }
            })
        if (running) {
            for (const w of windows.slice(0, 8))
                items.push({ text: w.title || w.name, icon: w.minimized ? "restore" : "maximize",
                             trigger: () => Toplevels.activate(w.id), separatorBefore: w === windows[0] })
            items.push({ text: windows.length > 1 ? qsTr("Close all windows") : qsTr("Close window"),
                         icon: "close", separatorBefore: true,
                         trigger: () => windows.forEach(w => Toplevels.close(w.id)) })
        }
        ShellState.openMenu(btn.mapToItem(null, btn.width / 2, 0).x, items)
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 2
        radius: Theme.radius
        color: mouse.pressed ? Theme.pressed : (mouse.containsMouse || btn.isActive) ? Theme.hover : "transparent"
        Behavior on color { ColorAnimation { duration: Theme.durationFast } }
    }
    AppIcon {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: btn.launching ? -4 : -1
        name: btn.iconName
        size: 24
        scale: mouse.pressed ? 0.86 : 1
        Behavior on scale { NumberAnimation { duration: Theme.durationFast } }
        Behavior on anchors.verticalCenterOffset { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }
    }
    Timer {
        id: launchTimer
        interval: 450
        onTriggered: btn.launching = false
    }
    // Running indicator: short pill; wider and accent-coloured when focused.
    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 3
        height: 3
        radius: 1.5
        width: btn.isActive ? 16 : 6
        color: btn.isActive ? Theme.accent : Theme.textTertiary
        visible: btn.running
        Behavior on width { NumberAnimation { duration: Theme.duration; easing.type: Theme.easing } }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        onClicked: (m) => {
            ShellState.hideTooltip()
            if (m.button === Qt.RightButton) btn.showMenu()
            else if (m.button === Qt.MiddleButton) { if (btn.entry.appId.length) Apps.launch(btn.entry.appId) }
            else btn.trigger()
        }
        onContainsMouseChanged: {
            if (containsMouse) tipTimer.restart()
            else { tipTimer.stop(); ShellState.hideTooltip() }
        }
    }
    Timer {
        id: tipTimer
        interval: 600
        onTriggered: {
            let text = btn.label
            if (btn.windows.length === 1 && btn.windows[0].title.length && btn.windows[0].title !== btn.label)
                text = btn.windows[0].title + " — " + btn.label
            else if (btn.windows.length > 1)
                text = qsTr("%1 — %2 windows").arg(btn.label).arg(btn.windows.length)
            ShellState.showTooltip(text, btn.mapToItem(null, btn.width / 2, 0).x)
        }
    }
}
