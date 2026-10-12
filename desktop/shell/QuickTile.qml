import QtQuick
import QtQuick.Layouts
import Meridian.Core
import Meridian.Shell

// Toggle tile in quick settings. With hasDetails, the right part opens a
// sub-page (e.g. the list of Wi-Fi networks).
Item {
    id: tile
    property string label
    property string sublabel
    property string iconName
    property bool checked: false
    property bool hasDetails: false
    signal toggled()
    signal detailsRequested()

    Layout.fillWidth: true
    implicitWidth: 104
    implicitHeight: 84
    opacity: enabled ? 1 : 0.5

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusLarge
        color: tile.checked ? (main.pressed ? Theme.accentPressed : main.containsMouse ? Theme.accentHover : Theme.accent)
                            : (main.pressed ? Theme.pressed : main.containsMouse ? Qt.tint(Theme.control, Theme.hover) : Theme.control)
        border.width: tile.checked ? 0 : 1
        border.color: Theme.controlBorder
        Behavior on color { ColorAnimation { duration: Theme.durationFast } }
    }

    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
        spacing: 6
        Icon {
            name: tile.iconName
            size: 18
            color: tile.checked ? Theme.onAccent : Theme.text
        }
        Text {
            width: parent.width
            text: tile.label
            elide: Text.ElideRight
            font.pixelSize: 13
            font.weight: Font.DemiBold
            color: tile.checked ? Theme.onAccent : Theme.text
        }
        Text {
            width: parent.width
            text: tile.sublabel
            elide: Text.ElideRight
            font.pixelSize: 11
            color: tile.checked ? Theme.alpha(Theme.onAccent, 0.8) : Theme.textSecondary
        }
    }

    MouseArea {
        id: main
        anchors.fill: parent
        hoverEnabled: true
        onClicked: tile.toggled()
    }
    // Details chevron
    Rectangle {
        visible: tile.hasDetails
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 6
        width: 26
        height: 26
        radius: Theme.radius
        color: details.containsMouse ? Theme.alpha(tile.checked ? Theme.onAccent : Theme.text, 0.15) : "transparent"
        Icon {
            anchors.centerIn: parent
            name: "chevron-right"
            size: 14
            color: tile.checked ? Theme.onAccent : Theme.text
        }
        MouseArea {
            id: details
            anchors.fill: parent
            hoverEnabled: true
            onClicked: tile.detailsRequested()
        }
    }
}
