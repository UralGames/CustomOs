import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core

// A question with a primary (accept) and a Cancel button.
Dialog {
    id: dialog
    property string text
    property string acceptText: qsTr("OK")

    // Sized from the window overlay: the parent layout's width would loop back.
    width: Math.min(460, Overlay.overlay ? Overlay.overlay.width - 48 : 460)
    modal: true

    // Fixed content size: a wrapping label sized by the popup would make the
    // dialog's implicit height depend on itself.
    contentWidth: width - leftPadding - rightPadding
    contentHeight: message.implicitHeight
    contentItem: Label {
        id: message
        width: dialog.contentWidth
        text: dialog.text
        wrapMode: Text.Wrap
        color: Theme.textSecondary
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        Button {
            text: dialog.acceptText
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }
}
