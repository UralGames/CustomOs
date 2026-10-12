import QtQuick
import Meridian.Core
import Meridian.Shell

// Base for taskbar flyouts: rounded translucent panel with shadow that
// slides and fades in. Escape closes.
FocusScope {
    id: flyout
    property bool open: false
    default property alias content: body.data
    signal closeRequested()

    visible: opacity > 0.01
    opacity: open ? 1 : 0
    property real slide: open ? 0 : 16
    transform: Translate { y: flyout.slide }
    Behavior on opacity { NumberAnimation { duration: Theme.duration; easing.type: Theme.easing } }
    Behavior on slide { NumberAnimation { duration: Theme.duration; easing.type: Theme.easing } }

    onOpenChanged: if (open) forceActiveFocus()
    Keys.onEscapePressed: closeRequested()

    BorderImage {
        anchors.fill: parent
        anchors.margins: -32
        anchors.topMargin: -28
        anchors.bottomMargin: -38
        source: "qrc:/images/shadow-popup.png"
        border { left: 46; right: 46; top: 46; bottom: 46 }
    }
    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusXL
        color: Theme.flyout
        border.color: Theme.border
        border.width: 1
    }
    // Clicks inside the panel must not reach the click-away layer.
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        onPressed: (mouse) => mouse.accepted = true
        onWheel: (wheel) => wheel.accepted = true
    }
    Item {
        id: body
        anchors.fill: parent
    }
}
