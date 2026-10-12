import QtQuick
import Meridian.Core
import Meridian.Shell

// Square taskbar button with an icon (start, task view).
Item {
    id: btn
    property string iconSource: ""
    property string symbolic: ""
    property bool highlighted: false
    property string tooltip: ""
    property string badge: ""
    signal clicked()

    width: 44
    height: 44

    Rectangle {
        anchors.fill: parent
        anchors.margins: 2
        radius: Theme.radius
        color: mouse.pressed ? Theme.pressed : (mouse.containsMouse || btn.highlighted) ? Theme.hover : "transparent"
        Behavior on color { ColorAnimation { duration: Theme.durationFast } }
    }
    Image {
        anchors.centerIn: parent
        visible: btn.iconSource.length > 0
        source: btn.iconSource
        width: 24
        height: 24
        sourceSize: Qt.size(48, 48)
        scale: mouse.pressed ? 0.86 : 1
        Behavior on scale { NumberAnimation { duration: Theme.durationFast } }
    }
    Icon {
        anchors.centerIn: parent
        visible: btn.symbolic.length > 0
        name: btn.symbolic
        size: 20
        scale: mouse.pressed ? 0.86 : 1
        Behavior on scale { NumberAnimation { duration: Theme.durationFast } }
    }
    Rectangle {
        visible: btn.badge.length > 0
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 6
        width: 14
        height: 14
        radius: 7
        color: Theme.accent
        Text {
            anchors.centerIn: parent
            text: btn.badge
            font.pixelSize: 9
            font.weight: Font.Bold
            color: Theme.onAccent
        }
    }
    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: { ShellState.hideTooltip(); btn.clicked() }
        onContainsMouseChanged: {
            if (containsMouse && btn.tooltip.length) tipTimer.restart()
            else { tipTimer.stop(); ShellState.hideTooltip() }
        }
    }
    Timer {
        id: tipTimer
        interval: 700
        onTriggered: ShellState.showTooltip(btn.tooltip, btn.mapToItem(null, btn.width / 2, 0).x)
    }
}
