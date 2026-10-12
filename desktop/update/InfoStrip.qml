import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core

// A one-line notice with an icon, in the accent tint.
Rectangle {
    id: strip
    property string iconName: "info"
    property string text

    radius: Theme.radiusLarge
    color: Theme.accentSoft
    implicitHeight: row.implicitHeight + 2 * Theme.spacingM

    RowLayout {
        id: row
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: Theme.spacingL
        anchors.rightMargin: Theme.spacingL
        spacing: 12
        Icon { name: strip.iconName; size: 18; color: Theme.accent; Layout.alignment: Qt.AlignTop }
        Label { Layout.fillWidth: true; text: strip.text; wrapMode: Text.Wrap }
    }
}
