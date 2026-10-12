import QtQuick
import QtQuick.Templates as T
import Meridian.Core

T.RadioButton {
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
        implicitWidth: 20
        implicitHeight: 20
        x: control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        radius: width / 2
        color: control.checked ? Theme.accent : control.hovered ? Theme.hover : Theme.inputBackground
        border.width: control.checked ? 0 : 1
        border.color: control.visualFocus ? Theme.accent : Theme.borderStrong
        opacity: control.enabled ? 1 : 0.5
        Rectangle {
            anchors.centerIn: parent
            width: control.down ? 6 : control.hovered ? 10 : 8
            height: width
            radius: width / 2
            color: Theme.onAccent
            visible: control.checked
            Behavior on width { NumberAnimation { duration: Theme.durationFast } }
        }
    }

    contentItem: Text {
        leftPadding: control.indicator.width + control.spacing
        text: control.text
        font: control.font
        color: control.enabled ? Theme.text : Theme.textDisabled
        verticalAlignment: Text.AlignVCenter
    }
}
