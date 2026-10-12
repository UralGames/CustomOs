import QtQuick
import QtQuick.Templates as T
import Meridian.Core

T.DialogButtonBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            (control.count === 1 ? implicitContentWidth * 2 : implicitContentWidth) + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    spacing: 8
    padding: 20
    alignment: Qt.AlignRight

    delegate: Button {
        width: Math.max(implicitWidth, 96)
        highlighted: DialogButtonBox.buttonRole === DialogButtonBox.AcceptRole
                     || DialogButtonBox.buttonRole === DialogButtonBox.YesRole
                     || DialogButtonBox.buttonRole === DialogButtonBox.ApplyRole
    }

    contentItem: ListView {
        implicitWidth: contentWidth
        model: control.contentModel
        spacing: control.spacing
        orientation: ListView.Horizontal
        boundsBehavior: Flickable.StopAtBounds
        snapMode: ListView.SnapToItem
    }

    background: Rectangle {
        implicitHeight: 72
        color: Theme.surfaceAlt
        radius: Theme.radiusXL
        // square off the top corners so it sits flush under the content
        Rectangle { width: parent.width; height: parent.radius; color: parent.color }
    }
}
