import QtQuick
import QtQuick.Templates as T
import Meridian.Core

T.TextField {
    id: control

    implicitWidth: implicitBackgroundWidth + leftInset + rightInset
                   || Math.max(contentWidth, placeholder.implicitWidth) + leftPadding + rightPadding
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding,
                             placeholder.implicitHeight + topPadding + bottomPadding)
    padding: 8
    leftPadding: 11
    rightPadding: 11
    font.pixelSize: Theme.fontBody
    color: enabled ? Theme.text : Theme.textDisabled
    selectionColor: Theme.selection
    selectedTextColor: Theme.text
    placeholderTextColor: Theme.textTertiary
    verticalAlignment: TextInput.AlignVCenter
    hoverEnabled: true

    Text {
        id: placeholder
        x: control.leftPadding
        y: control.topPadding
        width: control.width - (control.leftPadding + control.rightPadding)
        height: control.height - (control.topPadding + control.bottomPadding)
        text: control.placeholderText
        font: control.font
        color: control.placeholderTextColor
        verticalAlignment: control.verticalAlignment
        visible: !control.length && !control.preeditText.length
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: Theme.controlHeight
        radius: Theme.radius
        color: control.activeFocus ? Theme.surface : control.hovered ? Qt.tint(Theme.inputBackground, Theme.hover) : Theme.inputBackground
        border.width: 1
        border.color: control.activeFocus ? Theme.accent : Theme.controlBorder
        // Accent underline while focused
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: parent.radius
            anchors.rightMargin: parent.radius
            height: 2
            color: Theme.accent
            visible: control.activeFocus
        }
    }
}
