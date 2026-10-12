import QtQuick
import QtQuick.Templates as T
import Meridian.Core

T.ComboBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)
    leftPadding: 12
    rightPadding: 36
    hoverEnabled: true
    font.pixelSize: Theme.fontBody

    delegate: ItemDelegate {
        required property var model
        required property int index
        width: ListView.view.width
        text: control.textRole ? (Array.isArray(control.model) ? model.modelData[control.textRole] : model[control.textRole])
                               : model.modelData !== undefined ? model.modelData : model.display
        highlighted: control.highlightedIndex === index
        font.weight: control.currentIndex === index ? Font.DemiBold : Font.Normal
        hoverEnabled: control.hoverEnabled
    }

    indicator: Icon {
        x: control.width - width - 12
        y: control.topPadding + (control.availableHeight - height) / 2
        name: "chevron-down"
        size: 14
        color: control.enabled ? Theme.textSecondary : Theme.textDisabled
    }

    contentItem: Text {
        text: control.displayText
        font: control.font
        color: control.enabled ? Theme.text : Theme.textDisabled
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitWidth: 180
        implicitHeight: Theme.controlHeight
        radius: Theme.radius
        color: control.down ? Theme.pressed : control.hovered ? Qt.tint(Theme.control, Theme.hover) : Theme.control
        border.width: 1
        border.color: control.visualFocus ? Theme.accent : Theme.controlBorder
    }

    popup: T.Popup {
        y: control.height + 4
        width: control.width
        implicitHeight: Math.min(contentItem.implicitHeight + 8, 360)
        padding: 4

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.delegateModel
            currentIndex: control.highlightedIndex
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}
        }

        enter: Transition {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.durationFast }
        }

        background: Rectangle {
            color: Theme.panelSolid
            radius: Theme.radiusLarge
            border.color: Theme.border
            PopupShadow { spread: 20 }
        }
    }
}
