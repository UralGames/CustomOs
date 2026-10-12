import QtQuick
import QtQuick.Controls
import Meridian.Core

// Page and section headings with consistent typography.
Label {
    property int level: 1   // 1 = page title, 2 = section, 3 = group caption
    color: level === 3 ? Theme.textSecondary : Theme.text
    font.pixelSize: level === 1 ? Theme.fontTitle : level === 2 ? Theme.fontSubtitle : Theme.fontCaption
    font.weight: level === 3 ? Font.DemiBold : Font.DemiBold
    font.letterSpacing: level === 3 ? 0.3 : 0
    elide: Text.ElideRight
}
