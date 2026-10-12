import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Update

// "What's new": the release notes of the available version (Markdown from
// the release manifest, or the GitHub release text).
SectionCard {
    id: card
    required property var view
    property bool expanded: false
    readonly property bool isLong: notes.implicitHeight > 260

    Heading {
        text: qsTr("What's new in %1 %2").arg(card.view.osName).arg(card.view.s.availableVersion || "")
        level: 2
        Layout.fillWidth: true
    }

    Item {
        Layout.fillWidth: true
        implicitHeight: card.isLong && !card.expanded ? 260 : notes.implicitHeight
        clip: true
        Text {
            id: notes
            width: parent.width
            // Markdown rendered in C++ so links take the accent colour.
            text: Updates.renderNotes(card.view.s.releaseNotes || "", Theme.text, Theme.accent)
            textFormat: Text.RichText
            wrapMode: Text.Wrap
            color: Theme.text
            linkColor: Theme.accent
            font.pixelSize: Theme.fontBody
            lineHeight: 1.15
            onLinkActivated: link => Qt.openUrlExternally(link)
        }
        Rectangle {
            visible: card.isLong && !card.expanded
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 48
            gradient: Gradient {
                GradientStop { position: 0; color: Theme.alpha(Theme.surface, 0) }
                GradientStop { position: 1; color: Theme.surface }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Button {
            visible: card.isLong
            flat: true
            leftPadding: 0
            text: card.expanded ? qsTr("Show less") : qsTr("Show all")
            icon.name: card.expanded ? "chevron-up" : "chevron-down"
            onClicked: card.expanded = !card.expanded
        }
        Item { Layout.fillWidth: true }
        Button {
            visible: (card.view.s.releaseUrl || "").length > 0
            flat: true
            text: qsTr("Release page")
            icon.name: "open"
            onClicked: Qt.openUrlExternally(card.view.s.releaseUrl)
        }
    }
}
