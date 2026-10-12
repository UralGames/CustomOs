import QtQuick
import Meridian.Core
import Meridian.Shell

// Root of the shell: owns the layer-shell surfaces and routes actions coming
// from labwc key bindings (meridian-shell --action NAME).
Item {
    id: shell

    function handleAction(name) {
        switch (name) {
        case "ping": break
        case "launcher": ShellState.toggle("launcher"); break
        case "quick-settings": ShellState.toggle("quick"); break
        case "notifications": ShellState.toggle("calendar"); break
        case "task-view": ShellState.toggle("taskview"); break
        case "close-panel": ShellState.closePanel(); break
        case "show-desktop": ShellState.closePanel(); Toplevels.showDesktop(); break
        case "lock": ShellState.closePanel(); Session.lock(); break
        case "screenshot": ShellState.closePanel(); shotTimer.restart(); break
        case "files": Apps.launch("meridian-files.desktop"); break
        case "terminal": Apps.openTerminal(); break
        case "settings": Apps.launch("meridian-settings.desktop"); break
        case "monitor": Apps.launch("meridian-monitor.desktop"); break
        case "volume-up": Audio.changeVolume(5); break
        case "volume-down": Audio.changeVolume(-5); break
        case "volume-mute": Audio.toggleMute(); break
        case "desktop-next": Workspaces.next(); break
        case "desktop-prev": Workspaces.previous(); break
        case "reload-config": Session.writeCompositorConfig(); break
        default:
            if (name.startsWith("taskbar-"))
                taskbar.activateIndex(Number(name.substring(8)))
            else
                console.warn("meridian-shell: unknown action", name)
        }
    }

    Connections {
        target: ShellIpc
        function onAction(name) { shell.handleAction(name) }
    }
    Connections {
        target: Session
        function onScreenshotSaved(path) {
            Notifications.post(qsTr("Screenshot"), qsTr("Screenshot saved"),
                               qsTr("Saved to %1").arg(path.replace(Sys.homePath, "~")), "camera")
        }
        function onScreenshotFailed(reason) {
            Notifications.post(qsTr("Screenshot"), qsTr("Screenshot failed"), reason, "warning")
        }
    }
    Connections {
        target: Workspaces
        function onSwitched(index, name) { ShellState.osd("desktops", name) }
    }
    Connections {
        target: Apps
        function onLaunchFailed(name, reason) {
            Notifications.post(qsTr("System"), qsTr("Could not open %1").arg(name), reason, "warning")
        }
    }

    // Give panels a moment to fade before the screen is captured.
    Timer {
        id: shotTimer
        interval: 350
        onTriggered: Session.screenshot()
    }

    BackgroundWindow {}
    TaskbarWindow { id: taskbar }
    OverlayWindow {}
    ToastWindow {}
    OsdWindow {}
    TooltipWindow {}

    Component.onCompleted: {
        Session.start()
        if (Sys.isLive)
            Notifications.post(qsTr("Welcome"), qsTr("Welcome to %1").arg(SystemInfo.osName || "CustomOS"),
                               qsTr("This is a live session: changes are lost when you shut down. "
                                    + "To install, open “Install %1” from Start.").arg(SystemInfo.osName || "CustomOS"),
                               "customos-logo")
    }
}
