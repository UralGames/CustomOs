import QtQuick
import QtQuick.Templates as T
import Meridian.Core

T.Slider {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitHandleWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitHandleHeight + topPadding + bottomPadding)
    padding: 4
    hoverEnabled: true

    readonly property bool isHorizontal: orientation === Qt.Horizontal

    handle: Rectangle {
        x: control.leftPadding + (control.isHorizontal ? control.visualPosition * (control.availableWidth - width)
                                                      : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.isHorizontal ? (control.availableHeight - height) / 2
                                                     : control.visualPosition * (control.availableHeight - height))
        implicitWidth: 20
        implicitHeight: 20
        radius: 10
        color: Theme.dark ? "#3A4141" : "#FFFFFF"
        border.width: 1
        border.color: Theme.controlBorder
        Rectangle {
            anchors.centerIn: parent
            width: control.pressed ? 8 : control.hovered ? 12 : 10
            height: width
            radius: width / 2
            color: control.enabled ? Theme.accent : Theme.textDisabled
            Behavior on width { NumberAnimation { duration: Theme.durationFast } }
        }
    }

    background: Item {
        implicitWidth: control.isHorizontal ? 200 : 20
        implicitHeight: control.isHorizontal ? 20 : 200
        x: control.leftPadding
        y: control.topPadding
        width: control.availableWidth
        height: control.availableHeight

        Rectangle {
            x: control.isHorizontal ? 0 : (parent.width - width) / 2
            y: control.isHorizontal ? (parent.height - height) / 2 : 0
            width: control.isHorizontal ? parent.width : 4
            height: control.isHorizontal ? 4 : parent.height
            radius: 2
            color: Theme.borderStrong

            Rectangle {
                y: control.isHorizontal ? 0 : control.visualPosition * parent.height
                width: control.isHorizontal ? control.position * parent.width : 4
                height: control.isHorizontal ? 4 : control.position * parent.height
                radius: 2
                color: control.enabled ? Theme.accent : Theme.textDisabled
            }
        }
    }
}
