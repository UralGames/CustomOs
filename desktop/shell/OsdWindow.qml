import QtQuick
import QtQuick.Window
import Meridian.Core
import Meridian.Shell

// Small click-through surface for the volume / desktop indicator.
Window {
    id: win
    visible: false
    color: "transparent"
    width: 340
    height: 96
    title: "meridian-osd"

    Component.onCompleted: {
        LayerShell.setup(win, {
            layer: LayerShell.Overlay,
            anchors: LayerShell.AnchorBottom,
            exclusiveZone: 0,
            keyboard: LayerShell.KeyboardNone,
            marginBottom: 24,
            scope: "meridian-osd"
        })
    }

    function show() {
        if (!visible) visible = true
        hideTimer.restart()
        Qt.callLater(() => LayerShell.setInputRegion(win, []))
    }
    Timer {
        id: hideTimer
        interval: 1900
        onTriggered: win.visible = false
    }

    Osd {
        id: osd
        anchors.centerIn: parent
        suppressed: ShellState.panel === "quick"
        onShown: win.show()
    }
    Connections {
        target: ShellState
        function onOsd(icon, text) { osd.showText(icon, text) }
    }
}
