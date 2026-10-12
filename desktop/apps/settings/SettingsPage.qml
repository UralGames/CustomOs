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
    // Set by Main.qml: switches to another page by id.
    property var navigate: function(pageId) {}
    // Sub-pages show "Parent › Title"; the parent name goes back.
    property string parentTitle
    property string parentPage
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
        RowLayout {
            spacing: 4
            Layout.fillWidth: true
            Label {
                visible: page.parentTitle.length > 0
                text: page.parentTitle
                font.pixelSize: Theme.fontTitle
                font.weight: Font.DemiBold
                color: parentHover.hovered ? Theme.text : Theme.textSecondary
                HoverHandler { id: parentHover; cursorShape: Qt.PointingHandCursor }
                TapHandler { onTapped: page.navigate(page.parentPage) }
            }
            Icon {
                visible: page.parentTitle.length > 0
                name: "chevron-right"
                size: 18
                color: Theme.textSecondary
            }
            Heading { text: page.title; level: 1; Layout.fillWidth: true }
        }
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
