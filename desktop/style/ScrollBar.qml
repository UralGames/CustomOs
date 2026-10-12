import QtQuick
import QtQuick.Templates as T
import Meridian.Core

// Slim scrollbar that thickens while hovered or dragged. The hit area stays
// constant (10 px) so hovering does not change the layout.
T.ScrollBar {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    padding: 1
    visible: policy !== T.ScrollBar.AlwaysOff
    minimumSize: orientation === Qt.Horizontal ? height / width : width / height
    hoverEnabled: true

    readonly property bool expanded: control.hovered || control.pressed
    readonly property bool isVertical: orientation === Qt.Vertical

    contentItem: Item {
        implicitWidth: 10
        implicitHeight: 10
        opacity: control.policy === T.ScrollBar.AlwaysOn || (control.active && control.size < 1.0) ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.duration } }

        Rectangle {
            readonly property real thickness: control.expanded ? 6 : 3
            width: control.isVertical ? thickness : parent.width
            height: control.isVertical ? parent.height : thickness
            anchors.right: control.isVertical ? parent.right : undefined
            anchors.bottom: control.isVertical ? undefined : parent.bottom
            anchors.rightMargin: 1
            anchors.bottomMargin: 1
            radius: thickness / 2
            color: control.pressed ? Theme.textSecondary : Theme.textTertiary
            Behavior on width { NumberAnimation { duration: Theme.durationFast } }
            Behavior on height { NumberAnimation { duration: Theme.durationFast } }
        }
    }
}
