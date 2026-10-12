import QtQuick
import QtQuick.Templates as T
import Meridian.Core

T.Popup {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)
    padding: Theme.spacingL

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.durationFast }
        NumberAnimation { property: "scale"; from: 0.97; to: 1; duration: Theme.duration; easing.type: Theme.easing }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.durationFast }
    }

    background: Rectangle {
        color: Theme.panelSolid
        radius: Theme.radiusXL
        border.color: Theme.border
        border.width: 1
        PopupShadow {}
    }

    T.Overlay.modal: Rectangle {
        color: Theme.scrim
        Behavior on opacity { NumberAnimation { duration: Theme.duration } }
    }
    T.Overlay.modeless: Item {}
}
