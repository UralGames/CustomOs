import QtQuick
import QtQuick.Templates as T
import Meridian.Core

T.MenuItem {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    padding: 6
    leftPadding: 10
    rightPadding: 10
    spacing: 10
    hoverEnabled: true
    font.pixelSize: Theme.fontBody

    // Optional keyboard hint shown on the right ("Ctrl+S").
    property string shortcutText: ""
    // Optional icon tint (e.g. danger red for destructive actions).
    property color iconColor: !enabled ? Theme.textDisabled : Theme.text
    property color textColor: !enabled ? Theme.textDisabled : Theme.text

    contentItem: Item {
        implicitWidth: row.implicitWidth + (control.subMenu ? 20 : 0) + (hint.visible ? hint.implicitWidth + 24 : 0)
        implicitHeight: Math.max(row.implicitHeight, 22)
        Row {
            id: row
            anchors.verticalCenter: parent.verticalCenter
            spacing: control.spacing
            Item {
                width: 16; height: 16
                anchors.verticalCenter: parent.verticalCenter
                Icon {
                    anchors.centerIn: parent
                    name: control.checkable ? (control.checked ? "check" : "") : control.icon.name
                    color: control.checkable ? Theme.accent : control.iconColor
                    size: 16
                }
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: control.text
                font: control.font
                color: control.textColor
            }
        }
        Text {
            id: hint
            anchors.right: parent.right
            anchors.rightMargin: control.subMenu ? 20 : 0
            anchors.verticalCenter: parent.verticalCenter
            text: control.shortcutText
            visible: text.length > 0
            font.pixelSize: Theme.fontCaption
            color: Theme.textTertiary
        }
        Icon {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            name: "chevron-right"
            size: 14
            color: Theme.textSecondary
            visible: control.subMenu !== null
        }
    }

    arrow: Item {}
    indicator: Item {}

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 32
        radius: Theme.radius
        color: control.down ? Theme.pressed : control.highlighted ? Theme.hover : "transparent"
    }
}
