import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.folderlistmodel
import Meridian.Core

// In-app file picker used by the Text Editor and Settings, styled like the
// rest of the desktop. mode: "open", "save" or "folder".
Popup {
    id: chooser
    property string mode: "open"
    property string title: mode === "save" ? qsTr("Save as") : mode === "folder" ? qsTr("Choose a folder") : qsTr("Open")
    property string folder: Sys.homePath
    property var nameFilters: []
    property string fileName: ""
    property bool showHidden: false
    signal accepted(string path)

    function openAt(dir, name) {
        folder = dir && Sys.isDir(dir) ? dir : Sys.homePath
        fileName = name || ""
        nameField.text = fileName
        open()
    }

    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(760, (parent ? parent.width : 760) - 48)
    height: Math.min(520, (parent ? parent.height : 520) - 48)
    padding: 0

    function finish() {
        if (mode === "save") {
            const n = nameField.text.trim()
            if (!n.length) return
            accepted(folder.replace(/\/$/, "") + "/" + n)
        } else if (mode === "folder") {
            accepted(folder)
        } else {
            if (list.currentIndex < 0) return
            const p = folderModel.get(list.currentIndex, "filePath")
            if (folderModel.isFolder(list.currentIndex)) { folder = p; return }
            accepted(p)
        }
        close()
    }

    FolderListModel {
        id: folderModel
        folder: Sys.fileUrl(chooser.folder)
        showDirsFirst: true
        showDotAndDotDot: false
        showHidden: chooser.showHidden
        showFiles: chooser.mode !== "folder"
        nameFilters: chooser.nameFilters
        sortCaseSensitive: false
    }

    contentItem: ColumnLayout {
        spacing: 0

        // Header: title and path
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: Theme.spacingL
            spacing: Theme.spacingS
            Heading { text: chooser.title; level: 2; Layout.fillWidth: true }
            ToolButton {
                icon.name: chooser.showHidden ? "eye" : "eye-off"
                checkable: true
                checked: chooser.showHidden
                onToggled: chooser.showHidden = checked
                ToolTip.text: qsTr("Show hidden files")
                ToolTip.visible: hovered
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingL
            Layout.rightMargin: Theme.spacingL
            spacing: Theme.spacingS
            ToolButton {
                icon.name: "arrow-up"
                enabled: chooser.folder !== "/"
                onClicked: chooser.folder = Sys.parentDir(chooser.folder)
            }
            TextField {
                Layout.fillWidth: true
                text: chooser.folder
                onAccepted: if (Sys.isDir(text)) chooser.folder = text
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Theme.spacingL
            spacing: Theme.spacingM

            // Places
            ColumnLayout {
                Layout.preferredWidth: 150
                Layout.fillHeight: true
                spacing: 2
                Repeater {
                    model: [
                        { label: qsTr("Home"), icon: "home", path: Sys.homePath },
                        { label: qsTr("Desktop"), icon: "desktop", path: Sys.standardDir("desktop") },
                        { label: qsTr("Documents"), icon: "documents", path: Sys.standardDir("documents") },
                        { label: qsTr("Downloads"), icon: "downloads", path: Sys.standardDir("downloads") },
                        { label: qsTr("Pictures"), icon: "pictures", path: Sys.standardDir("pictures") },
                        { label: qsTr("Computer"), icon: "drive", path: "/" }
                    ]
                    delegate: ItemDelegate {
                        required property var modelData
                        Layout.fillWidth: true
                        visible: Sys.isDir(modelData.path)
                        text: modelData.label
                        icon.name: modelData.icon
                        highlighted: chooser.folder === modelData.path
                        onClicked: chooser.folder = modelData.path
                    }
                }
                Item { Layout.fillHeight: true }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: Theme.radius
                color: Theme.inputBackground
                border.color: Theme.border
                clip: true

                ListView {
                    id: list
                    anchors.fill: parent
                    anchors.margins: 4
                    model: folderModel
                    currentIndex: -1
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar {}
                    delegate: ItemDelegate {
                        required property int index
                        required property string fileName
                        required property bool fileIsDir
                        required property string filePath
                        width: ListView.view.width
                        text: fileName
                        icon.name: fileIsDir ? "folder" : "file"
                        highlighted: ListView.isCurrentItem
                        onClicked: {
                            list.currentIndex = index
                            if (!fileIsDir && chooser.mode === "save")
                                nameField.text = fileName
                        }
                        onDoubleClicked: {
                            if (fileIsDir) { chooser.folder = filePath; list.currentIndex = -1 }
                            else chooser.finish()
                        }
                    }
                    Label {
                        anchors.centerIn: parent
                        visible: list.count === 0
                        text: qsTr("This folder is empty")
                        color: Theme.textSecondary
                    }
                }
            }
        }

        // Footer
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: footer.implicitHeight + Theme.spacingL * 2
            color: Theme.surfaceAlt
            radius: Theme.radiusLarge
            Rectangle { anchors.top: parent.top; width: parent.width; height: parent.radius; color: parent.color }
            RowLayout {
                id: footer
                anchors.fill: parent
                anchors.margins: Theme.spacingL
                spacing: Theme.spacingS
                TextField {
                    id: nameField
                    visible: chooser.mode === "save"
                    Layout.fillWidth: true
                    placeholderText: qsTr("File name")
                    onAccepted: chooser.finish()
                }
                Item { Layout.fillWidth: chooser.mode !== "save" }
                Button { text: qsTr("Cancel"); onClicked: chooser.close() }
                Button {
                    text: chooser.mode === "save" ? qsTr("Save") : chooser.mode === "folder" ? qsTr("Select") : qsTr("Open")
                    highlighted: true
                    enabled: chooser.mode === "save" ? nameField.text.trim().length > 0
                           : chooser.mode === "folder" ? true : list.currentIndex >= 0
                    onClicked: chooser.finish()
                }
            }
        }
    }
}
