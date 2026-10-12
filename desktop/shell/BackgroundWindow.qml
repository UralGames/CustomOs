import QtQuick
import QtQuick.Controls
import QtQuick.Window
import Meridian.Core
import Meridian.Shell

// Wallpaper surface (background layer) and the desktop's right-click menu.
Window {
    id: win
    visible: false
    color: "black"
    width: Screen.width
    height: Screen.height
    title: "meridian-background"

    Component.onCompleted: {
        LayerShell.setup(win, {
            layer: LayerShell.Background,
            anchors: LayerShell.AnchorTop | LayerShell.AnchorBottom | LayerShell.AnchorLeft | LayerShell.AnchorRight,
            exclusiveZone: -1,
            keyboard: LayerShell.KeyboardNone,
            scope: "meridian-background"
        })
        visible = true
    }

    Wallpaper {
        anchors.fill: parent
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onPressed: (mouse) => {
            ShellState.closePanel()
            if (mouse.button === Qt.RightButton)
                desktopMenu.popup()
        }
    }

    Menu {
        id: desktopMenu
        MenuItem {
            text: qsTr("Open Terminal")
            icon.name: "terminal"
            onTriggered: Apps.openTerminal()
        }
        MenuItem {
            text: qsTr("Open Files")
            icon.name: "folder"
            onTriggered: Apps.launch("meridian-files.desktop")
        }
        MenuSeparator {}
        MenuItem {
            text: qsTr("Next wallpaper")
            icon.name: "image"
            onTriggered: {
                const list = Sys.wallpapers()
                if (!list.length) return
                const i = list.indexOf(Settings.wallpaper)
                Settings.wallpaper = list[(i + 1) % list.length]
            }
        }
        MenuItem {
            text: Theme.dark ? qsTr("Switch to light mode") : qsTr("Switch to dark mode")
            icon.name: Theme.dark ? "sun" : "moon"
            onTriggered: Settings.darkMode = !Settings.darkMode
        }
        MenuSeparator {}
        MenuItem {
            text: qsTr("Display settings")
            icon.name: "display"
            onTriggered: Apps.launch("meridian-settings.desktop", ["display"])
        }
        MenuItem {
            text: qsTr("Personalise")
            icon.name: "palette"
            onTriggered: Apps.launch("meridian-settings.desktop", ["appearance"])
        }
    }
}
