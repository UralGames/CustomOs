import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Shell

// Start menu: search, pinned apps, alphabetical list of all apps, the user
// and power actions.
Flyout {
    id: launcher
    property bool showAll: false

    width: 620
    height: Math.min(700, parent ? parent.height - 24 : 700)

    onOpenChanged: {
        if (open) {
            search.text = ""
            showAll = false
            search.forceActiveFocus()
        }
    }

    readonly property var results: search.text.trim().length ? Apps.search(search.text) : []

    function launchApp(id) {
        launcher.closeRequested()
        Apps.launch(id)
    }
    function pinToggle(listKey, id) {
        const list = (Settings[listKey] || []).slice()
        const i = list.indexOf(id)
        if (i >= 0) list.splice(i, 1)
        else list.push(id)
        Settings[listKey] = list
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 24
        spacing: 0

        // Search
        TextField {
            id: search
            Layout.fillWidth: true
            Layout.leftMargin: 28
            Layout.rightMargin: 28
            Layout.preferredHeight: 38
            leftPadding: 40
            placeholderText: qsTr("Search apps")
            Icon {
                anchors.left: parent.left
                anchors.leftMargin: 13
                anchors.verticalCenter: parent.verticalCenter
                name: "search"
                size: 16
                color: Theme.textSecondary
            }
            Keys.onReturnPressed: {
                if (launcher.results.length) launcher.launchApp(launcher.results[resultList.currentIndex >= 0 ? resultList.currentIndex : 0].appId)
            }
            Keys.onDownPressed: resultList.incrementCurrentIndex()
            Keys.onUpPressed: resultList.decrementCurrentIndex()
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 20
            currentIndex: search.text.trim().length ? 2 : launcher.showAll ? 1 : 0

            // 0: pinned grid
            ColumnLayout {
                spacing: 8
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 40
                    Layout.rightMargin: 28
                    Heading { text: qsTr("Pinned"); level: 2; font.pixelSize: 15; Layout.fillWidth: true }
                    Button {
                        text: qsTr("All apps")
                        flat: true
                        icon.name: "chevron-right"
                        onClicked: launcher.showAll = true
                        LayoutMirroring.enabled: true
                        LayoutMirroring.childrenInherit: true
                    }
                }
                GridView {
                    id: pinnedGrid
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.leftMargin: 28
                    Layout.rightMargin: 28
                    cellWidth: Math.floor(width / 6)
                    cellHeight: 104
                    clip: true
                    interactive: contentHeight > height
                    model: (Settings.launcherPinned || []).filter(id => Apps.exists(id))
                    delegate: LauncherTile {
                        required property var modelData
                        width: pinnedGrid.cellWidth
                        height: pinnedGrid.cellHeight
                        app: Apps.get(modelData)
                        onLaunch: launcher.launchApp(modelData)
                        onContextRequested: (x, y) => { appMenu.appId = modelData; appMenu.popup() }
                    }
                    Label {
                        anchors.centerIn: parent
                        visible: pinnedGrid.count === 0
                        text: qsTr("Pin apps here from “All apps” (right-click an app).")
                        color: Theme.textSecondary
                    }
                }
            }

            // 1: all apps
            ColumnLayout {
                spacing: 8
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 40
                    Layout.rightMargin: 28
                    Heading { text: qsTr("All apps"); level: 2; font.pixelSize: 15; Layout.fillWidth: true }
                    Button {
                        text: qsTr("Back")
                        flat: true
                        icon.name: "chevron-left"
                        onClicked: launcher.showAll = false
                    }
                }
                ListView {
                    id: allList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.leftMargin: 28
                    Layout.rightMargin: 28
                    clip: true
                    model: Apps
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar {}
                    section.property: "name"
                    section.criteria: ViewSection.FirstCharacter
                    section.delegate: Text {
                        required property string section
                        width: ListView.view.width
                        text: section.toUpperCase()
                        leftPadding: 12
                        topPadding: 10
                        bottomPadding: 4
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        color: Theme.accent
                    }
                    delegate: ItemDelegate {
                        required property string appId
                        required property string name
                        required property string iconName
                        required property string comment
                        width: ListView.view.width
                        height: 44
                        contentItem: Row {
                            spacing: 12
                            AppIcon { anchors.verticalCenter: parent.verticalCenter; name: iconName; size: 26 }
                            Column {
                                anchors.verticalCenter: parent.verticalCenter
                                Text { text: name; color: Theme.text; font.pixelSize: 14 }
                            }
                        }
                        onClicked: launcher.launchApp(appId)
                        TapHandler {
                            acceptedButtons: Qt.RightButton
                            onTapped: { appMenu.appId = appId; appMenu.popup() }
                        }
                    }
                }
            }

            // 2: search results
            ColumnLayout {
                spacing: 8
                Heading {
                    Layout.leftMargin: 40
                    text: launcher.results.length ? qsTr("Best matches") : qsTr("No apps match “%1”").arg(search.text)
                    level: 2
                    font.pixelSize: 15
                }
                ListView {
                    id: resultList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.leftMargin: 28
                    Layout.rightMargin: 28
                    clip: true
                    model: launcher.results
                    currentIndex: 0
                    boundsBehavior: Flickable.StopAtBounds
                    delegate: ItemDelegate {
                        required property var modelData
                        required property int index
                        width: ListView.view.width
                        height: 52
                        highlighted: ListView.isCurrentItem
                        contentItem: Row {
                            spacing: 12
                            AppIcon { anchors.verticalCenter: parent.verticalCenter; name: modelData.iconName; size: 30 }
                            Column {
                                anchors.verticalCenter: parent.verticalCenter
                                Text { text: modelData.name; color: Theme.text; font.pixelSize: 14; font.weight: Font.Medium }
                                Text {
                                    text: modelData.comment || modelData.genericName || ""
                                    visible: text.length > 0
                                    color: Theme.textSecondary
                                    font.pixelSize: 12
                                }
                            }
                        }
                        onClicked: launcher.launchApp(modelData.appId)
                    }
                }
            }
        }

        // Footer: user and power
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 64
            color: Theme.dark ? Qt.rgba(1, 1, 1, 0.03) : Qt.rgba(0, 0, 0, 0.025)
            radius: Theme.radiusXL
            Rectangle { width: parent.width; height: 1; color: Theme.divider }
            Rectangle { anchors.top: parent.top; width: parent.width; height: parent.radius; color: parent.color; z: -1 }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 36
                anchors.rightMargin: 28
                spacing: 12

                ItemDelegate {
                    Layout.preferredHeight: 44
                    contentItem: Row {
                        spacing: 12
                        Rectangle {
                            width: 32; height: 32; radius: 16
                            anchors.verticalCenter: parent.verticalCenter
                            color: Theme.accent
                            Text {
                                anchors.centerIn: parent
                                text: (Settings.userDisplayName || Sys.userFullName()).charAt(0).toUpperCase()
                                color: Theme.onAccent
                                font.pixelSize: 15
                                font.weight: Font.DemiBold
                            }
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: Settings.userDisplayName || Sys.userFullName()
                            color: Theme.text
                            font.pixelSize: 14
                        }
                    }
                    onClicked: { launcher.closeRequested(); Apps.launch("meridian-settings.desktop", ["user"]) }
                }
                Item { Layout.fillWidth: true }
                ToolButton {
                    icon.name: "settings"
                    onClicked: { launcher.closeRequested(); Apps.launch("meridian-settings.desktop") }
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Settings")
                }
                ToolButton {
                    id: powerButton
                    icon.name: "power"
                    onClicked: powerMenu.popup(powerButton, 0, -powerMenu.implicitHeight - 4)
                    ToolTip.visible: hovered && !powerMenu.visible
                    ToolTip.text: qsTr("Power")
                }
            }
        }
    }

    Menu {
        id: powerMenu
        MenuItem { text: qsTr("Lock"); icon.name: "lock"; shortcutText: "Super+L"; onTriggered: { launcher.closeRequested(); Session.lock() } }
        MenuItem { text: qsTr("Sign out"); icon.name: "logout"; onTriggered: Session.logout() }
        MenuSeparator {}
        MenuItem { text: qsTr("Restart"); icon.name: "restart"; onTriggered: Session.reboot() }
        MenuItem { text: qsTr("Shut down"); icon.name: "power"; onTriggered: Session.powerOff() }
    }

    Menu {
        id: appMenu
        property string appId: ""
        readonly property bool inStart: (Settings.launcherPinned || []).indexOf(appId) >= 0
        readonly property bool inTaskbar: (Settings.pinnedApps || []).indexOf(appId) >= 0
        MenuItem {
            text: appMenu.inStart ? qsTr("Unpin from Start") : qsTr("Pin to Start")
            icon.name: appMenu.inStart ? "unpin" : "pin"
            onTriggered: launcher.pinToggle("launcherPinned", appMenu.appId)
        }
        MenuItem {
            text: appMenu.inTaskbar ? qsTr("Unpin from taskbar") : qsTr("Pin to taskbar")
            icon.name: appMenu.inTaskbar ? "unpin" : "pin"
            onTriggered: launcher.pinToggle("pinnedApps", appMenu.appId)
        }
    }
}
