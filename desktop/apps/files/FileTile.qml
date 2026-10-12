import QtQuick
import Meridian.Core

// One item in the icon view (with thumbnails for images).
Item {
    id: tile
    required property int index
    required property string fileName
    required property string filePath
    required property string iconName
    required property string category
    required property bool isHidden
    property bool selected: false
    property bool isCut: false
    signal clicked(int modifiers)
    signal doubleClicked()
    signal contextRequested()

    Rectangle {
        anchors.fill: parent
        anchors.margins: 4
        radius: Theme.radiusLarge
        color: tile.selected ? (mouse.containsMouse ? Qt.tint(Theme.accentSoft, Theme.hover) : Theme.accentSoft)
                             : mouse.containsMouse ? Theme.hover : "transparent"
        border.width: tile.selected ? 1 : 0
        border.color: Theme.alpha(Theme.accent, 0.5)
    }
    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 12
        spacing: 8
        opacity: tile.isCut || tile.isHidden ? 0.55 : 1
        FileIcon {
            anchors.horizontalCenter: parent.horizontalCenter
            category: tile.category
            iconName: tile.iconName
            path: tile.filePath
            size: 56
            thumbnail: true
        }
        Text {
            width: tile.width - 16
            horizontalAlignment: Text.AlignHCenter
            text: tile.fileName
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            maximumLineCount: 2
            elide: Text.ElideRight
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
            if (m.button === Qt.RightButton) tile.contextRequested()
            else tile.clicked(m.modifiers)
        }
        onDoubleClicked: (m) => { if (m.button === Qt.LeftButton) tile.doubleClicked() }
    }
}
