import QtQuick
import QtQuick.Controls
import Meridian.Core
import Meridian.Shell

// Task view: virtual desktops at the top and the open windows below.
FocusScope {
    id: tv
    property bool open: false
    signal closeRequested()

    visible: opacity > 0.01
    opacity: open ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: Theme.duration; easing.type: Theme.easing } }

    readonly property var windows: open ? (Toplevels.revision, Toplevels.windows()) : []

    Rectangle {
        anchors.fill: parent
        color: Theme.dark ? Qt.rgba(0.05, 0.07, 0.07, 0.88) : Qt.rgba(0.92, 0.94, 0.93, 0.88)
    }
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        onClicked: tv.closeRequested()
    }

    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 40
        spacing: 36

        // ---- desktops ----
        Column {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 12
            visible: Workspaces.available
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Desktops")
                font.pixelSize: 15
                font.weight: Font.DemiBold
                color: Theme.text
            }
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 12
                Repeater {
                    model: tv.open ? Workspaces.workspaces : []
                    delegate: Item {
                        id: deskCard
                        required property var modelData
                        width: 168
                        height: 96
                        Rectangle {
                            anchors.fill: parent
                            radius: Theme.radiusLarge
                            color: deskMouse.containsMouse ? Theme.surfaceAlt : Theme.surface
                            border.width: deskCard.modelData.active ? 2 : 1
                            border.color: deskCard.modelData.active ? Theme.accent : Theme.border
                        }
                        Column {
                            anchors.centerIn: parent
                            spacing: 6
                            Icon {
                                anchors.horizontalCenter: parent.horizontalCenter
                                name: "desktop"
                                size: 22
                                color: deskCard.modelData.active ? Theme.accent : Theme.textSecondary
                            }
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: deskCard.modelData.name
                                color: Theme.text
                                font.pixelSize: 13
                                font.weight: deskCard.modelData.active ? Font.DemiBold : Font.Normal
                            }
                        }
                        MouseArea {
                            id: deskMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: { Workspaces.activate(deskCard.modelData.index); tv.closeRequested() }
                        }
                    }
                }
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Ctrl+Super+←/→ switches desktops • Shift+Super+←/→ moves the focused window")
                font.pixelSize: 12
                color: Theme.textSecondary
            }
        }

        // ---- windows ----
        Column {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 12
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: tv.windows.length ? qsTr("Open windows") : qsTr("No open windows")
                font.pixelSize: 15
                font.weight: Font.DemiBold
                color: Theme.text
            }
            Flow {
                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.min(tv.width - 120, Math.max(1, tv.windows.length) * (232 + spacing))
                spacing: 16
                Repeater {
                    model: tv.windows
                    delegate: Item {
                        id: card
                        required property var modelData
                        width: 232
                        height: 132
                        Rectangle {
                            anchors.fill: parent
                            radius: Theme.radiusLarge
                            color: cardMouse.containsMouse ? Theme.surfaceAlt : Theme.surface
                            border.width: card.modelData.activated ? 2 : 1
                            border.color: card.modelData.activated ? Theme.accent : Theme.border
                            opacity: card.modelData.minimized ? 0.75 : 1
                        }
                        Column {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 10
                            AppIcon { name: card.modelData.iconName; size: 40 }
                            Text {
                                width: parent.width
                                text: card.modelData.title || card.modelData.name
                                elide: Text.ElideRight
                                maximumLineCount: 2
                                wrapMode: Text.Wrap
                                font.pixelSize: 13
                                font.weight: Font.Medium
                                color: Theme.text
                            }
                            Text {
                                text: card.modelData.name + (card.modelData.minimized ? qsTr(" • minimised") : "")
                                font.pixelSize: 11
                                color: Theme.textSecondary
                            }
                        }
                        MouseArea {
                            id: cardMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: { tv.closeRequested(); Toplevels.activate(card.modelData.id) }
                        }
                        Rectangle {
                            visible: cardMouse.containsMouse || closeMouse.containsMouse
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 8
                            width: 26
                            height: 26
                            radius: Theme.radius
                            color: closeMouse.containsMouse ? "#C8392F" : Theme.surfaceAlt
                            Icon {
                                anchors.centerIn: parent
                                name: "close"
                                size: 12
                                color: closeMouse.containsMouse ? "#FFFFFF" : Theme.text
                            }
                            MouseArea {
                                id: closeMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: Toplevels.close(card.modelData.id)
                            }
                        }
                    }
                }
            }
        }
    }
}
