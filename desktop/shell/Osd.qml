import QtQuick
import Meridian.Core
import Meridian.Shell

// Brief on-screen indicator for volume changes and desktop switches.
Item {
    id: osd
    property bool suppressed: false
    property string mode: "volume"     // volume | text
    property string iconName: ""
    property string text: ""
    signal shown()

    width: 280
    height: 52
    opacity: 0
    visible: opacity > 0.01
    Behavior on opacity { NumberAnimation { duration: Theme.duration } }

    function showText(icon, msg) {
        mode = "text"
        iconName = icon
        text = msg
        opacity = 1
        hideTimer.restart()
        shown()
    }

    Connections {
        target: Audio
        function onVolumeAdjusted() {
            if (osd.suppressed) return
            osd.mode = "volume"
            osd.opacity = 1
            hideTimer.restart()
            osd.shown()
        }
    }
    Timer {
        id: hideTimer
        interval: 1600
        onTriggered: osd.opacity = 0
    }

    BorderImage {
        anchors.fill: parent
        anchors.margins: -24
        anchors.bottomMargin: -30
        source: "qrc:/images/shadow-popup.png"
        border { left: 46; right: 46; top: 46; bottom: 46 }
    }
    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: Theme.panelSolid
        border.color: Theme.border
    }
    Row {
        anchors.centerIn: parent
        spacing: 14
        visible: osd.mode === "volume"
        Icon {
            anchors.verticalCenter: parent.verticalCenter
            size: 18
            name: Audio.muted || Audio.volume === 0 ? "volume-mute"
                : Audio.volume < 34 ? "volume-low" : Audio.volume < 67 ? "volume-medium" : "volume-high"
        }
        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: 170
            height: 4
            radius: 2
            color: Theme.borderStrong
            Rectangle {
                width: parent.width * (Audio.muted ? 0 : Audio.volume / 100)
                height: parent.height
                radius: 2
                color: Theme.accent
                Behavior on width { NumberAnimation { duration: Theme.durationFast } }
            }
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            width: 32
            text: Audio.muted ? "–" : Audio.volume
            color: Theme.text
            font.pixelSize: 13
        }
    }
    Row {
        anchors.centerIn: parent
        spacing: 10
        visible: osd.mode === "text"
        Icon { anchors.verticalCenter: parent.verticalCenter; name: osd.iconName; size: 18 }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: osd.text
            color: Theme.text
            font.pixelSize: 14
            font.weight: Font.Medium
        }
    }
}
