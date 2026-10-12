import QtQuick
import QtQuick.Templates as T
import Meridian.Core

T.TextArea {
    id: control
    // Set framed: true for a bordered input box; editors use it frameless.
    property bool framed: true

    implicitWidth: Math.max(contentWidth + leftPadding + rightPadding,
                            implicitBackgroundWidth + leftInset + rightInset,
                            placeholder.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(contentHeight + topPadding + bottomPadding,
                             implicitBackgroundHeight + topInset + bottomInset,
                             placeholder.implicitHeight + topPadding + bottomPadding)
    padding: 10
    font.pixelSize: Theme.fontBody
    color: enabled ? Theme.text : Theme.textDisabled
    selectionColor: Theme.selection
    selectedTextColor: Theme.text
    placeholderTextColor: Theme.textTertiary
    hoverEnabled: true

    Text {
        id: placeholder
        x: control.leftPadding
        y: control.topPadding
        width: control.width - (control.leftPadding + control.rightPadding)
        text: control.placeholderText
        font: control.font
        color: control.placeholderTextColor
        visible: !control.length && !control.preeditText.length
        elide: Text.ElideRight
        wrapMode: Text.Wrap
    }

    background: Rectangle {
        visible: control.framed
        implicitWidth: 200
        implicitHeight: 80
        radius: Theme.radius
        color: Theme.inputBackground
        border.width: 1
        border.color: control.activeFocus ? Theme.accent : Theme.controlBorder
    }
}
