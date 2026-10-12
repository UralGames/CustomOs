import QtQuick
import QtQuick.Templates as T
import Meridian.Core

// List row. When highlighted it shows a soft accent fill and a small accent
// pill on the left edge.
T.ItemDelegate {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    padding: 8
    leftPadding: 12
    rightPadding: 12
    spacing: 10
    hoverEnabled: true
    font.pixelSize: Theme.fontBody

    // Optional secondary line under the text.
    property string subtitle: ""

    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: Math.max(row.implicitHeight, 20)
        Row {
            id: row
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width
            spacing: control.spacing
            Icon {
                id: ic
                anchors.verticalCenter: parent.verticalCenter
                name: control.icon.name
                color: control.enabled ? (control.highlighted ? Theme.accent : Theme.text) : Theme.textDisabled
                size: 16
                visible: name.length > 0
            }
            Column {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - (ic.visible ? ic.width + control.spacing : 0)
                Text {
                    width: parent.width
                    text: control.text
                    font: control.font
                    color: control.enabled ? Theme.text : Theme.textDisabled
                    elide: Text.ElideRight
                }
                Text {
                    width: parent.width
                    text: control.subtitle
                    visible: text.length > 0
                    font.pixelSize: Theme.fontCaption
                    color: Theme.textSecondary
                    elide: Text.ElideRight
                }
            }
        }
    }

    background: Rectangle {
        implicitHeight: 36
        radius: Theme.radius
        color: control.down ? Theme.pressed
             : control.highlighted ? (control.hovered ? Qt.tint(Theme.accentSoft, Theme.hover) : Theme.accentSoft)
             : control.hovered ? Theme.hover : "transparent"
        Rectangle {
            visible: control.highlighted
            width: 3
            height: parent.height * 0.45
            radius: 1.5
            anchors.left: parent.left
            anchors.leftMargin: 1
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.accent
        }
    }
}
