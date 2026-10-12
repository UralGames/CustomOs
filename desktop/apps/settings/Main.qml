import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Settings

ApplicationWindow {
    id: win
    property string startPage: "system"
    property string page: startPage

    width: 1040
    height: 720
    minimumWidth: 760
    minimumHeight: 480
    visible: true
    title: page === "updates" ? qsTr("Updates")
           : pages.find(p => p.id === page) ? pages.find(p => p.id === page).label : ""

    readonly property var pages: [
        { id: "system", label: qsTr("System"), icon: "info" },
        { id: "display", label: qsTr("Display"), icon: "display" },
        { id: "sound", label: qsTr("Sound"), icon: "volume-high" },
        { id: "network", label: qsTr("Network"), icon: "globe" },
        { id: "appearance", label: qsTr("Personalisation"), icon: "palette" },
        { id: "taskbar", label: qsTr("Taskbar"), icon: "apps" },
        { id: "multitasking", label: qsTr("Multitasking"), icon: "desktops" },
        { id: "input", label: qsTr("Keyboard & mouse"), icon: "keyboard" },
        { id: "notifications", label: qsTr("Notifications"), icon: "bell" },
        { id: "time", label: qsTr("Date & time"), icon: "clock" },
        { id: "user", label: qsTr("Your account"), icon: "user" }
    ]

    // Messages from pages (applied changes, errors)
    property string toast: ""
    property bool toastError: false
    function notify(msg, isError) {
        toast = msg
        toastError = isError === true
        toastTimer.restart()
    }
    Timer { id: toastTimer; interval: 5000; onTriggered: win.toast = "" }

    Shortcut { sequences: [StandardKey.Quit, StandardKey.Close]; onActivated: Qt.quit() }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ---- navigation ----
        Rectangle {
            Layout.preferredWidth: 260
            Layout.fillHeight: true
            color: Theme.sidebar
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 2
                // Account summary
                ItemDelegate {
                    Layout.fillWidth: true
                    Layout.bottomMargin: 10
                    implicitHeight: 64
                    onClicked: win.page = "user"
                    contentItem: RowLayout {
                        spacing: 12
                        Rectangle {
                            width: 40; height: 40; radius: 20
                            color: Theme.accent
                            Text {
                                anchors.centerIn: parent
                                text: (Settings.userDisplayName || Sys.userFullName()).charAt(0).toUpperCase()
                                color: Theme.onAccent
                                font.pixelSize: 18
                                font.weight: Font.DemiBold
                            }
                        }
                        ColumnLayout {
                            spacing: 0
                            Label { text: Settings.userDisplayName || Sys.userFullName(); font.weight: Font.DemiBold; elide: Text.ElideRight; Layout.fillWidth: true }
                            Label { text: Sys.userName + "@" + Sys.hostName(); color: Theme.textSecondary; font.pixelSize: Theme.fontCaption; elide: Text.ElideRight; Layout.fillWidth: true }
                        }
                    }
                }
                Repeater {
                    model: win.pages
                    delegate: ItemDelegate {
                        required property var modelData
                        Layout.fillWidth: true
                        text: modelData.label
                        icon.name: modelData.icon
                        // Updates is a sub-page of System.
                        highlighted: win.page === modelData.id || (modelData.id === "system" && win.page === "updates")
                        onClicked: win.page = modelData.id
                    }
                }
                Item { Layout.fillHeight: true }
            }
        }

        // ---- page ----
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: win.toast.length ? 42 : 0
                visible: Layout.preferredHeight > 0
                color: win.toastError ? Theme.dangerBackground : Theme.accentSoft
                Behavior on Layout.preferredHeight { NumberAnimation { duration: Theme.duration } }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 8
                    Icon { name: win.toastError ? "warning" : "check"; size: 16; color: win.toastError ? Theme.danger : Theme.accent }
                    Label { Layout.fillWidth: true; text: win.toast; elide: Text.ElideRight }
                    ToolButton { icon.name: "close"; onClicked: win.toast = "" }
                }
            }

            Loader {
                id: loader
                Layout.fillWidth: true
                Layout.fillHeight: true
                source: {
                    switch (win.page) {
                    case "display": return "DisplayPage.qml"
                    case "sound": return "SoundPage.qml"
                    case "network": return "NetworkPage.qml"
                    case "appearance": return "AppearancePage.qml"
                    case "taskbar": return "TaskbarPage.qml"
                    case "multitasking": return "MultitaskingPage.qml"
                    case "input": return "InputPage.qml"
                    case "notifications": return "NotificationsPage.qml"
                    case "time": return "TimePage.qml"
                    case "user": return "UserPage.qml"
                    case "updates": return "UpdatesPage.qml"
                    default: return "SystemPage.qml"
                    }
                }
                onLoaded: {
                    item.notify = win.notify
                    item.navigate = function(p) { win.page = p }
                }
            }
        }
    }
}
