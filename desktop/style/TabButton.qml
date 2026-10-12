import QtQuick
import QtQuick.Templates as T
import Meridian.Core

T.TabButton {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    padding: 8
    leftPadding: 14
    rightPadding: 14
    hoverEnabled: true
    font.pixelSize: Theme.fontBody

    contentItem: Text {
        text: control.text
        font.pixelSize: control.font.pixelSize
        font.weight: control.checked ? Font.DemiBold : Font.Normal
        color: control.checked ? Theme.text : Theme.textSecondary
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitHeight: 36
        radius: Theme.radius
        color: control.down ? Theme.pressed : control.hovered ? Theme.hover : "transparent"
    }
}
