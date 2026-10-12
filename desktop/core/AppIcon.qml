import QtQuick

// An application icon by XDG icon name (or absolute path).
Item {
    id: icon
    property string name
    property int size: 32

    width: size
    height: size
    implicitWidth: size
    implicitHeight: size

    Image {
        anchors.fill: parent
        sourceSize: Qt.size(icon.size * 2, icon.size * 2)
        smooth: true
        mipmap: true
        asynchronous: true
        source: "image://icon/app/" + (icon.name.length ? icon.name : "meridian-app")
    }
}
