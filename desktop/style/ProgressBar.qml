import QtQuick
import QtQuick.Templates as T
import Meridian.Core

T.ProgressBar {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    contentItem: Item {
        implicitWidth: 200
        implicitHeight: 4
        clip: true

        Rectangle {
            visible: !control.indeterminate
            width: control.visualPosition * parent.width
            height: parent.height
            radius: 2
            color: Theme.accent
            Behavior on width { NumberAnimation { duration: Theme.duration } }
        }

        Rectangle {
            id: runner
            visible: control.indeterminate
            width: parent.width * 0.35
            height: parent.height
            radius: 2
            color: Theme.accent
            XAnimator on x {
                running: control.indeterminate && control.visible
                from: -runner.width
                to: control.width
                duration: 1300
                loops: Animation.Infinite
                easing.type: Easing.InOutQuad
            }
        }
    }

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 4
        y: (control.height - height) / 2
        height: 4
        radius: 2
        color: Theme.borderStrong
    }
}
