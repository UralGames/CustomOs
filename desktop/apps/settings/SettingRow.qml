import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core

// One setting: icon, title + description on the left, control on the right.
RowLayout {
    id: row
    property string title
    property string description
    property string iconName
    default property alias control: trailing.data

    Layout.fillWidth: true
    spacing: 14

    Icon {
        visible: row.iconName.length > 0
        name: row.iconName
        size: 18
        color: Theme.textSecondary
        Layout.alignment: Qt.AlignVCenter
    }
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2
        Label { text: row.title; Layout.fillWidth: true; wrapMode: Text.Wrap }
        Label {
            visible: row.description.length > 0
            text: row.description
            color: Theme.textSecondary
            font.pixelSize: Theme.fontCaption
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }
    }
    RowLayout {
        id: trailing
        spacing: 8
        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
    }
}
