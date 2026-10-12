import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core

// Scrollable page with a title; children are stacked in a column.
ScrollView {
    id: page
    property string title
    property string subtitle
    property var notify: function(msg, isError) {}
    default property alias content: column.data

    contentWidth: availableWidth
    contentHeight: column.implicitHeight + 72
    clip: true

    ColumnLayout {
        id: column
        x: 32
        y: 0
        width: Math.min(page.availableWidth - 64, 860)
        spacing: 16

        Item { Layout.preferredHeight: 12 }
        Heading { text: page.title; level: 1 }
        Label {
            visible: page.subtitle.length > 0
            text: page.subtitle
            color: Theme.textSecondary
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            Layout.topMargin: -8
        }
        Item { Layout.preferredHeight: 4 }
    }
}
