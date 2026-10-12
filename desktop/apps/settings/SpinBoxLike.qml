import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core

// Compact − value + stepper.
RowLayout {
    id: s
    property int value: 0
    property int from: 0
    property int to: 10
    signal modified(int value)
    spacing: 4
    Button { icon.name: "minus"; enabled: s.value > s.from; onClicked: s.modified(s.value - 1) }
    Label { text: s.value; Layout.preferredWidth: 32; horizontalAlignment: Text.AlignHCenter; font.weight: Font.DemiBold }
    Button { icon.name: "plus"; enabled: s.value < s.to; onClicked: s.modified(s.value + 1) }
}
