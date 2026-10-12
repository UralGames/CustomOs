import QtQuick
import QtQuick.Templates as T
import Meridian.Core

T.Dialog {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding,
                            implicitHeaderWidth, implicitFooterWidth)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding
                             + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0)
                             + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0))
    padding: 24
    topPadding: 8
    spacing: 8
    modal: true
    anchors.centerIn: T.Overlay.overlay

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.durationFast }
        NumberAnimation { property: "scale"; from: 0.96; to: 1; duration: Theme.duration; easing.type: Theme.easing }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.durationFast }
    }

    background: Rectangle {
        color: Theme.panelSolid
        radius: Theme.radiusXL
        border.color: Theme.border
        PopupShadow {}
    }

    header: Text {
        text: control.title
        visible: control.title.length > 0
        elide: Text.ElideRight
        padding: 24
        bottomPadding: 0
        font.pixelSize: Theme.fontSubtitle
        font.weight: Font.DemiBold
        color: Theme.text
    }

    footer: DialogButtonBox {
        visible: count > 0
    }

    T.Overlay.modal: Rectangle {
        color: Theme.scrim
        Behavior on opacity { NumberAnimation { duration: Theme.duration } }
    }
    T.Overlay.modeless: Item {}
}
