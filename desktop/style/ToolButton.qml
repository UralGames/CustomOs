import QtQuick
import QtQuick.Templates as T
import Meridian.Core

// Compact, borderless icon button (optionally with text).
T.ToolButton {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    padding: 8
    spacing: 6
    hoverEnabled: true
    font.pixelSize: Theme.fontBody

    readonly property color foreground: !enabled ? Theme.textDisabled
                                       : checked ? Theme.accent : Theme.text

    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: row.implicitHeight
        Row {
            id: row
            anchors.centerIn: parent
            spacing: control.spacing
            Icon {
                anchors.verticalCenter: parent.verticalCenter
                name: control.icon.name
                color: control.foreground
                size: control.icon.width > 0 && control.icon.width !== 24 ? control.icon.width : 16
                visible: name.length > 0
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: control.text
                font: control.font
                color: control.foreground
                visible: text.length > 0 && control.display !== T.AbstractButton.IconOnly
            }
        }
    }

    background: Rectangle {
        implicitWidth: 32
        implicitHeight: 32
        radius: Theme.radius
        color: control.down ? Theme.pressed
             : control.checked ? Theme.accentSoft
             : control.hovered ? Theme.hover : "transparent"
        border.width: control.visualFocus ? 1 : 0
        border.color: Theme.accent
        Behavior on color { ColorAnimation { duration: Theme.durationFast } }
    }
}
