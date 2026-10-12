import QtQuick
import Meridian.Core

// A Meridian symbolic icon, tinted with `color`. The wrapper Item reports
// `size` as its implicit size (an Image would report its 2x source size).
Item {
    id: icon
    property string name
    property color color: Theme.text
    property int size: 16

    width: size
    height: size
    implicitWidth: size
    implicitHeight: size

    Image {
        anchors.fill: parent
        sourceSize: Qt.size(icon.size * 2, icon.size * 2)   // render at 2x for crisp scaling
        smooth: true
        asynchronous: false
        source: icon.name.length ? "image://icon/sym/" + icon.name + "?c=" + Theme.hex(icon.color) : ""
    }
}
