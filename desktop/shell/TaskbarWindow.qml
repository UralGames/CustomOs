import QtQuick
import QtQuick.Window
import Meridian.Core
import Meridian.Shell

// The taskbar surface: top layer, along the bottom edge, reserving its
// height so maximised windows stop above it.
Window {
    id: win
    color: "transparent"
    width: Screen.width
    height: ShellState.taskbarHeight
    title: "meridian-taskbar"

    function activateIndex(i) { bar.activateIndex(i) }

    Component.onCompleted: {
        LayerShell.setup(win, {
            layer: LayerShell.Top,
            anchors: LayerShell.AnchorBottom | LayerShell.AnchorLeft | LayerShell.AnchorRight,
            exclusiveZone: ShellState.taskbarHeight,
            keyboard: LayerShell.KeyboardNone,
            scope: "meridian-taskbar"
        })
        ready = true
    }
    property bool ready: false
    // Get out of the way of full-screen applications (video, games).
    visible: ready && !Toplevels.fullscreenActive

    Taskbar {
        id: bar
        anchors.fill: parent
    }
}
