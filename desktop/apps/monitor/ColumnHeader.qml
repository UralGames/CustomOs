import QtQuick
import Meridian.Core

// Sortable column header for the process list.
Item {
    id: h
    property string text
    property string key
    property var model
    property bool alignRight: false
    readonly property bool active: model && model.sortKey === key
    implicitHeight: 32

    Row {
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: h.alignRight ? parent.right : undefined
        spacing: 4
        Text {
            text: h.text
            font.pixelSize: 12
            font.weight: h.active ? Font.DemiBold : Font.Medium
            color: h.active ? Theme.text : Theme.textSecondary
        }
        Icon {
            anchors.verticalCenter: parent.verticalCenter
            visible: h.active
            name: h.model && h.model.sortDescending ? "chevron-down" : "chevron-up"
            size: 12
            color: Theme.textSecondary
        }
    }
    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (h.active) h.model.sortDescending = !h.model.sortDescending
            else h.model.sortKey = h.key
        }
    }
}
