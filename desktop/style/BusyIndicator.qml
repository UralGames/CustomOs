import QtQuick
import QtQuick.Templates as T
import Meridian.Core

// Spinning arc.
T.BusyIndicator {
    id: control
    implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
    implicitHeight: contentItem.implicitHeight + topPadding + bottomPadding
    padding: 2

    contentItem: Item {
        implicitWidth: 28
        implicitHeight: 28
        opacity: control.running ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: Theme.duration } }

        Canvas {
            id: arc
            anchors.fill: parent
            property color stroke: Theme.accent
            onStrokeChanged: requestPaint()
            onPaint: {
                const ctx = getContext("2d")
                ctx.reset()
                ctx.lineWidth = Math.max(2, width / 10)
                ctx.lineCap = "round"
                ctx.strokeStyle = stroke
                const r = (Math.min(width, height) - ctx.lineWidth) / 2
                ctx.beginPath()
                ctx.arc(width / 2, height / 2, r, 0, Math.PI * 1.35)
                ctx.stroke()
            }
            RotationAnimator on rotation {
                from: 0; to: 360
                duration: 900
                loops: Animation.Infinite
                running: control.running && control.visible
            }
        }
    }
}
