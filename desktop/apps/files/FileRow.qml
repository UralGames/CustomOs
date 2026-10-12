import QtQuick
import QtQuick.Layouts
import Meridian.Core

// One row in the details view.
Item {
    id: row
    required property int index
    required property string fileName
    required property string filePath
    required property bool isDir
    required property string sizeText
    required property string modifiedText
    required property string typeText
    required property string iconName
    required property string category
    required property bool isHidden
    required property bool isLink
    property bool selected: false
    property bool isCut: false
    signal clicked(int modifiers)
    signal doubleClicked()
    signal contextRequested()

    height: 36

    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: 4
        anchors.rightMargin: 4
        radius: Theme.radius
        color: row.selected ? (mouse.containsMouse ? Qt.tint(Theme.accentSoft, Theme.hover) : Theme.accentSoft)
                            : mouse.containsMouse ? Theme.hover : "transparent"
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        spacing: 0
        opacity: row.isCut || row.isHidden ? 0.55 : 1

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            FileIcon { category: row.category; iconName: row.iconName; path: row.filePath; size: 20 }
            Text {
                Layout.fillWidth: true
                text: row.fileName
                elide: Text.ElideMiddle
                color: Theme.text
                font.pixelSize: 13
            }
            Icon { visible: row.isLink; name: "arrow-right"; size: 12; color: Theme.textTertiary }
        }
        Text {
            Layout.preferredWidth: 170
            text: row.modifiedText
            elide: Text.ElideRight
            color: Theme.textSecondary
            font.pixelSize: 13
        }
        Text {
            Layout.preferredWidth: 160
            text: row.typeText
            elide: Text.ElideRight
            color: Theme.textSecondary
            font.pixelSize: 13
        }
        Text {
            Layout.preferredWidth: 90
            text: row.sizeText
            horizontalAlignment: Text.AlignRight
            color: Theme.textSecondary
            font.pixelSize: 13
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: (m) => {
            if (m.button === Qt.RightButton) row.contextRequested()
            else row.clicked(m.modifiers)
        }
        onDoubleClicked: (m) => { if (m.button === Qt.LeftButton) row.doubleClicked() }
    }
}
