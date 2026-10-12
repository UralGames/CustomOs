import QtQuick
import QtQuick.Window
import Meridian.Core
import Meridian.Shell

// Transparent overlay-layer surface covering the screen above the taskbar.
// It is mapped only while a panel is open, takes the keyboard, and closes
// the panel when you click outside it.
Window {
    id: win
    visible: false
    color: "transparent"
    width: Screen.width
    height: Screen.height - ShellState.taskbarHeight
    title: "meridian-panels"

    readonly property bool wanted: ShellState.panel !== ""
    readonly property real panelX: Settings.taskbarAlignment === "left" ? 12 : 0

    onWantedChanged: {
        if (wanted) {
            hideTimer.stop()
            visible = true
            contentRoot.forceActiveFocus()
        } else {
            hideTimer.restart()
        }
    }
    // Keep the surface until the closing fade has finished.
    Timer {
        id: hideTimer
        interval: Theme.duration + 60
        onTriggered: if (!win.wanted) win.visible = false
    }

    Component.onCompleted: {
        LayerShell.setup(win, {
            layer: LayerShell.Overlay,
            anchors: LayerShell.AnchorTop | LayerShell.AnchorBottom | LayerShell.AnchorLeft | LayerShell.AnchorRight,
            exclusiveZone: 0,
            keyboard: LayerShell.KeyboardExclusive,
            scope: "meridian-panels"
        })
    }

    FocusScope {
        id: contentRoot
        anchors.fill: parent
        focus: true
        Keys.onEscapePressed: ShellState.closePanel()

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            onPressed: ShellState.closePanel()
        }

        TaskView {
            anchors.fill: parent
            open: ShellState.panel === "taskview"
            onCloseRequested: ShellState.closePanel()
        }

        Launcher {
            id: launcher
            open: ShellState.panel === "launcher"
            x: Settings.taskbarAlignment === "left" ? 12 : Math.round((win.width - width) / 2)
            y: win.height - height - 12
            onCloseRequested: ShellState.closePanel()
        }

        QuickSettings {
            open: ShellState.panel === "quick"
            x: win.width - width - 12
            y: win.height - height - 12
            onCloseRequested: ShellState.closePanel()
        }

        CalendarFlyout {
            open: ShellState.panel === "calendar"
            x: win.width - width - 12
            y: win.height - height - 12
            onCloseRequested: ShellState.closePanel()
        }

        ContextMenuPanel {
            open: ShellState.panel === "menu"
            items: ShellState.menuItems
            x: Math.max(8, Math.min(win.width - width - 8, ShellState.menuX - width / 2))
            y: win.height - height - 8
            onCloseRequested: ShellState.closePanel()
        }
    }
}
