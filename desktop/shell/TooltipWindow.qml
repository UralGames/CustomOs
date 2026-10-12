import QtQuick
import QtQuick.Window
import Meridian.Core
import Meridian.Shell

// Tooltip for taskbar items. A tiny click-through surface placed just
// above the taskbar; re-created for each tooltip so its size always fits.
Window {
    id: win
    visible: false
    color: "transparent"
    title: "meridian-tooltip"
    width: Math.ceil(label.implicitWidth) + 24
    height: 34

    property bool configured: false
    readonly property string text: ShellState.tooltipText

    Component.onCompleted: {
        LayerShell.setup(win, {
            layer: LayerShell.Overlay,
            anchors: LayerShell.AnchorBottom | LayerShell.AnchorLeft,
            exclusiveZone: 0,
            keyboard: LayerShell.KeyboardNone,
            marginBottom: 6,
            scope: "meridian-tooltip"
        })
        configured = true
    }

    onTextChanged: {
        if (!configured) return
        visible = false
        if (text.length === 0) return
        label.text = text
        const w = Math.ceil(label.implicitWidth) + 24
        const left = Math.max(4, Math.min(Screen.width - w - 4, Math.round(ShellState.tooltipX - w / 2)))
        LayerShell.setMargins(win, left, 0, 0, 6)
        visible = true
        Qt.callLater(() => LayerShell.setInputRegion(win, []))
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 2
        radius: Theme.radius
        color: Theme.panelSolid
        border.color: Theme.borderStrong
        Text {
            id: label
            anchors.centerIn: parent
            font.pixelSize: 12
            color: Theme.text
        }
    }
}
