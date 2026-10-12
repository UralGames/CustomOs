import QtQuick
import QtQuick.Templates as T
import Meridian.Core

// Standard button. `highlighted: true` makes it the accent (primary) button;
// `flat: true` removes the fill until hovered.
T.Button {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    padding: 6
    leftPadding: text.length ? 16 : 8
    rightPadding: text.length ? 16 : 8
    spacing: 8
    font.pixelSize: Theme.fontBody
    font.weight: Font.Medium
    hoverEnabled: true

    readonly property color foreground: !enabled ? Theme.textDisabled
                                       : highlighted ? Theme.onAccent : Theme.text

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
                size: 16
                visible: name.length > 0
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: control.text
                font: control.font
                color: control.foreground
                visible: text.length > 0
            }
        }
    }

    background: Rectangle {
        implicitWidth: 80
        implicitHeight: Theme.controlHeight
        radius: Theme.radius
        color: {
            if (control.highlighted) {
                if (!control.enabled) return Theme.alpha(Theme.accent, 0.4)
                return control.down ? Theme.accentPressed : control.hovered ? Theme.accentHover : Theme.accent
            }
            if (control.down) return Theme.pressed
            if (control.hovered || control.checked) return Theme.hover
            return control.flat ? "transparent" : Theme.control
        }
        border.width: control.flat || control.highlighted ? 0 : 1
        border.color: control.visualFocus ? Theme.accent : Theme.controlBorder
        Behavior on color { ColorAnimation { duration: Theme.durationFast } }
    }
}
