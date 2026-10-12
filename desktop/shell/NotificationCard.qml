import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Shell

// One notification, used both in the centre and as a toast.
Rectangle {
    id: card
    property var data_: ({})
    property bool toast: false
    signal dismissed()
    signal clicked()
    signal actionClicked(string key)

    readonly property string iconRef: data_.appIcon || ""
    implicitHeight: layout.implicitHeight + 24
    radius: Theme.radiusLarge
    color: toast ? Theme.panelSolid : Theme.surface
    border.color: Theme.border
    border.width: 1

    HoverHandler { id: hover }
    TapHandler { onTapped: card.clicked() }

    ColumnLayout {
        id: layout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Item {
                id: iconBox
                width: 18; height: 18
                // Icon kinds: absolute file path, one of Meridian's symbolic
                // icons (used by the shell's own messages), or an app icon name.
                readonly property bool isFile: card.iconRef.startsWith("/")
                readonly property bool isSymbolic: card.iconRef.length === 0
                    || ["image", "warning", "power", "restart", "info", "bell", "camera"].indexOf(card.iconRef) >= 0
                Image {
                    anchors.fill: parent
                    visible: iconBox.isFile
                    source: visible ? Sys.fileUrl(card.iconRef) : ""
                    sourceSize: Qt.size(36, 36)
                }
                Icon {
                    anchors.fill: parent
                    visible: !iconBox.isFile && iconBox.isSymbolic
                    name: card.iconRef.length ? card.iconRef : "bell"
                    size: 18
                    color: Theme.textSecondary
                }
                AppIcon {
                    anchors.fill: parent
                    visible: !iconBox.isFile && !iconBox.isSymbolic
                    name: visible ? card.iconRef : ""
                    size: 18
                }
            }
            Text {
                Layout.fillWidth: true
                text: card.data_.appName || ""
                font.pixelSize: 12
                color: Theme.textSecondary
                elide: Text.ElideRight
            }
            Text {
                text: card.data_.time ? Qt.formatTime(card.data_.time, "HH:mm") : ""
                font.pixelSize: 11
                color: Theme.textTertiary
                visible: !hover.hovered
            }
            Rectangle {
                visible: hover.hovered
                width: 22; height: 22
                radius: Theme.radius
                color: closeArea.containsMouse ? Theme.hover : "transparent"
                Icon { anchors.centerIn: parent; name: "close"; size: 12 }
                MouseArea { id: closeArea; anchors.fill: parent; hoverEnabled: true; onClicked: card.dismissed() }
            }
        }
        Text {
            Layout.fillWidth: true
            text: card.data_.summary || ""
            visible: text.length > 0
            font.pixelSize: 14
            font.weight: Font.DemiBold
            color: Theme.text
            wrapMode: Text.Wrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }
        Text {
            Layout.fillWidth: true
            text: card.data_.body || ""
            visible: text.length > 0
            font.pixelSize: 13
            color: Theme.textSecondary
            wrapMode: Text.Wrap
            maximumLineCount: card.toast ? 3 : 6
            elide: Text.ElideRight
            textFormat: Text.PlainText
        }
        Flow {
            Layout.fillWidth: true
            Layout.topMargin: 4
            spacing: 6
            visible: (card.data_.actions || []).length > 0
            Repeater {
                model: card.data_.actions || []
                delegate: Button {
                    required property var modelData
                    text: modelData.label
                    onClicked: card.actionClicked(modelData.key)
                }
            }
        }
    }
}
