import QtQuick
import Meridian.Core
import Meridian.Shell

// Wide tray button that hosts arbitrary content (status icons, clock).
Item {
    id: btn
    property Item contentItem
    property bool highlighted: false
    property string tooltip: ""
    signal clicked()

    width: (contentItem ? contentItem.implicitWidth : 0) + 20
    height: 40

    onContentItemChanged: if (contentItem) { contentItem.parent = btn; contentItem.anchors.centerIn = btn }

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: mouse.pressed ? Theme.pressed : (mouse.containsMouse || btn.highlighted) ? Theme.hover : "transparent"
        Behavior on color { ColorAnimation { duration: Theme.durationFast } }
        z: -1
    }
    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: { ShellState.hideTooltip(); btn.clicked() }
        onContainsMouseChanged: {
            if (containsMouse && btn.tooltip.length && !btn.highlighted) tipTimer.restart()
            else { tipTimer.stop(); ShellState.hideTooltip() }
        }
    }
    Timer {
        id: tipTimer
        interval: 900
        onTriggered: ShellState.showTooltip(btn.tooltip, btn.mapToItem(null, btn.width / 2, 0).x)
    }
}
