import QtQuick
import QtQuick.Templates as T
import Meridian.Core

T.Switch {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)
    padding: 4
    spacing: 10
    hoverEnabled: true
    font.pixelSize: Theme.fontBody

    indicator: Rectangle {
        implicitWidth: 40
        implicitHeight: 20
        x: control.text.length ? control.leftPadding : (control.width - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        radius: height / 2
        color: control.checked ? (control.hovered ? Theme.accentHover : Theme.accent)
                               : (control.hovered ? Theme.hover : "transparent")
        border.width: control.checked ? 0 : 1
        border.color: control.visualFocus ? Theme.accent : Theme.textSecondary
        opacity: control.enabled ? 1 : 0.45
        Behavior on color { ColorAnimation { duration: Theme.durationFast } }

        Rectangle {
            readonly property real knob: control.down ? 14 : control.hovered ? 13 : 12
            width: control.down ? knob + 3 : knob
            height: knob
            radius: knob / 2
            y: (parent.height - height) / 2
            x: control.checked ? parent.width - width - 4 : 4
            color: control.checked ? Theme.onAccent : Theme.textSecondary
            Behavior on x { NumberAnimation { duration: Theme.duration; easing.type: Theme.easing } }
            Behavior on width { NumberAnimation { duration: Theme.durationFast } }
        }
    }

    contentItem: Text {
        leftPadding: control.text.length ? control.indicator.width + control.spacing : 0
        text: control.text
        font: control.font
        color: control.enabled ? Theme.text : Theme.textDisabled
        verticalAlignment: Text.AlignVCenter
    }
}
