import QtQuick
import Meridian.Core
import Meridian.Shell

// Icon + label tile in the launcher's pinned grid.
Item {
    id: tile
    property var app: ({})
    signal launch()
    signal contextRequested(real x, real y)

    Rectangle {
        anchors.fill: parent
        anchors.margins: 3
        radius: Theme.radius
        color: mouse.pressed ? Theme.pressed : mouse.containsMouse ? Theme.hover : "transparent"
        Behavior on color { ColorAnimation { duration: Theme.durationFast } }
    }
    Column {
        anchors.centerIn: parent
        spacing: 8
        AppIcon {
            anchors.horizontalCenter: parent.horizontalCenter
            name: tile.app.iconName || "meridian-app"
            size: 36
            scale: mouse.pressed ? 0.9 : 1
            Behavior on scale { NumberAnimation { duration: Theme.durationFast } }
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            width: tile.width - 12
            horizontalAlignment: Text.AlignHCenter
            text: tile.app.name || ""
            elide: Text.ElideRight
            wrapMode: Text.Wrap
            maximumLineCount: 2
            lineHeight: 0.95
            font.pixelSize: 12
            color: Theme.text
        }
    }
    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: (m) => {
            if (m.button === Qt.RightButton) tile.contextRequested(m.x, m.y)
            else tile.launch()
        }
    }
}
