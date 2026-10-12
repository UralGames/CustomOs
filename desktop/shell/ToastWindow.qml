import QtQuick
import QtQuick.Window
import Meridian.Core
import Meridian.Shell

// Notification pop-ups, bottom-right above the taskbar. The surface is
// click-through except where a notification is drawn.
Window {
    id: win
    visible: false
    color: "transparent"
    width: 404
    height: 660
    title: "meridian-toasts"

    property bool configured: false
    readonly property bool wanted: configured && toasts.count > 0 && ShellState.panel === ""

    Component.onCompleted: {
        LayerShell.setup(win, {
            layer: LayerShell.Overlay,
            anchors: LayerShell.AnchorBottom | LayerShell.AnchorRight,
            exclusiveZone: 0,
            keyboard: LayerShell.KeyboardNone,
            marginRight: 8,
            marginBottom: 8,
            scope: "meridian-notifications"
        })
        configured = true
    }
    onWantedChanged: {
        if (wanted) visible = true
        else hideTimer.restart()
    }
    Timer {
        id: hideTimer
        interval: Theme.duration + 60
        onTriggered: if (!win.wanted) win.visible = false
    }

    function updateInputRegion() {
        if (visible) LayerShell.setInputRegion(win, toasts.cardRects())
    }
    onVisibleChanged: Qt.callLater(updateInputRegion)

    Toasts {
        id: toasts
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 12
        suppressed: ShellState.panel === "calendar"
        visible: ShellState.panel === ""
        onLayoutChanged: Qt.callLater(win.updateInputRegion)
    }
}
