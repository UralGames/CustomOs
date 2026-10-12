import QtQuick
import QtQuick.Templates as T
import Meridian.Core

T.ToolBar {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)
    padding: 6
    leftPadding: 10
    rightPadding: 10
    background: Rectangle {
        implicitHeight: 48
        color: Theme.window
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.divider
        }
    }
}
