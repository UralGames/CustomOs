import QtQuick
import Meridian.Core

// Area chart of the last 60 samples.
Canvas {
    id: g
    property var values: []
    property real maxValue: 100     // 0 = auto-scale
    property color lineColor: Theme.accent

    onValuesChanged: requestPaint()
    onLineColorChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        const ctx = getContext("2d")
        ctx.reset()
        const n = values.length
        if (n < 2) return
        let max = maxValue
        if (max <= 0) {
            max = 1
            for (let i = 0; i < n; ++i) max = Math.max(max, values[i])
            max *= 1.15
        }
        // grid
        ctx.strokeStyle = Theme.divider
        ctx.lineWidth = 1
        for (let k = 1; k < 4; ++k) {
            const y = Math.round(height * k / 4) + 0.5
            ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(width, y); ctx.stroke()
        }
        const step = width / (n - 1)
        const yOf = v => height - Math.min(1, v / max) * (height - 2) - 1
        ctx.beginPath()
        ctx.moveTo(0, height)
        for (let i = 0; i < n; ++i) ctx.lineTo(i * step, yOf(values[i]))
        ctx.lineTo(width, height)
        ctx.closePath()
        ctx.fillStyle = Qt.rgba(lineColor.r, lineColor.g, lineColor.b, 0.16)
        ctx.fill()
        ctx.beginPath()
        for (let i = 0; i < n; ++i) {
            if (i === 0) ctx.moveTo(0, yOf(values[0]))
            else ctx.lineTo(i * step, yOf(values[i]))
        }
        ctx.strokeStyle = lineColor
        ctx.lineWidth = 2
        ctx.stroke()
    }
}
