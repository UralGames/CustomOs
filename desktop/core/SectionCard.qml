import QtQuick
import QtQuick.Layouts
import Meridian.Core

// A rounded content card used to group settings rows and panels.
// Children are laid out in a column; use `spacing`/`padding` to tune.
Rectangle {
    id: card
    default property alias content: column.data
    property int padding: Theme.spacingL
    property alias spacing: column.spacing

    color: Theme.surface
    radius: Theme.radiusLarge
    border.color: Theme.border
    border.width: 1
    implicitHeight: column.implicitHeight + padding * 2
    implicitWidth: column.implicitWidth + padding * 2

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.margins: card.padding
        spacing: Theme.spacingM
    }
}
