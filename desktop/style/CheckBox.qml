import QtQuick
import QtQuick.Templates as T
import Meridian.Core

T.CheckBox {
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
        radius: Theme.radiusSmall
        color: control.checkState !== Qt.Unchecked
               ? (control.down ? Theme.accentPressed : control.hovered ? Theme.accentHover : Theme.accent)
               : (control.hovered ? Theme.hover : Theme.inputBackground)
        border.width: control.checkState !== Qt.Unchecked ? 0 : 1
        border.color: control.visualFocus ? Theme.accent : Theme.borderStrong
        opacity: control.enabled ? 1 : 0.5

        Icon {
            anchors.centerIn: parent
            name: control.checkState === Qt.PartiallyChecked ? "minus" : "check"
            color: Theme.onAccent
            size: 14
            visible: control.checkState !== Qt.Unchecked
        }
    }

    contentItem: Text {
        leftPadding: control.indicator.width + control.spacing
        text: control.text
        font: control.font
        color: control.enabled ? Theme.text : Theme.textDisabled
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.Wrap
    }
}
