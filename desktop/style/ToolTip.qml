import QtQuick
import QtQuick.Templates as T
import Meridian.Core

T.ToolTip {
    id: control

    x: parent ? (parent.width - implicitWidth) / 2 : 0
    y: -implicitHeight - 6
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)
    margins: 6
    padding: 6
    leftPadding: 10
    rightPadding: 10
    delay: 600
    timeout: 6000
    closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutsideParent | T.Popup.CloseOnReleaseOutsideParent

    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.durationFast } }
    exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.durationFast } }

    contentItem: Text {
        text: control.text
        font.pixelSize: Theme.fontCaption
        color: Theme.text
        wrapMode: Text.Wrap
    }

    background: Rectangle {
        color: Theme.panelSolid
        radius: Theme.radius
        border.color: Theme.borderStrong
    }
}
