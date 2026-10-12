import QtQuick
import QtQuick.Templates as T
import Meridian.Core

T.TabBar {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)
    spacing: 4

    contentItem: ListView {
        model: control.contentModel
        currentIndex: control.currentIndex
        spacing: control.spacing
        orientation: ListView.Horizontal
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.AutoFlickIfNeeded
        snapMode: ListView.SnapToItem
        highlightMoveDuration: Theme.duration
        highlightResizeDuration: Theme.duration
        highlight: Item {
            z: 2
            Rectangle {
                height: 3
                radius: 1.5
                width: parent.width * 0.4
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                color: Theme.accent
            }
        }
    }

    background: Item {}
}
