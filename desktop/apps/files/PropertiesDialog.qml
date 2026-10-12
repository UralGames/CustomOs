import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Files

// File / folder properties.
Dialog {
    id: dlg
    property var info: ({})
    property string sizeText: ""

    function show(path) {
        info = FileOps.properties(path)
        if (info.isDir) {
            sizeText = qsTr("Calculating…")
            FileOps.computeSize(path)
        } else {
            sizeText = Sys.formatBytes(info.size) + "  (" + Number(info.size).toLocaleString(Qt.locale(), "f", 0) + qsTr(" bytes)")
        }
        open()
    }
    Connections {
        target: FileOps
        function onSizeComputed(path, bytes, files, folders) {
            if (path !== dlg.info.path) return
            dlg.sizeText = qsTr("%1, %2 files, %3 folders").arg(Sys.formatBytes(bytes)).arg(files).arg(folders)
        }
    }

    title: qsTr("Properties")
    width: 500
    standardButtons: Dialog.Close

    contentItem: ColumnLayout {
        spacing: 14
        RowLayout {
            spacing: 14
            FileIcon {
                size: 48
                category: dlg.info.isDir ? "folder" : "other"
                iconName: "file"
            }
            ColumnLayout {
                spacing: 2
                Label { text: dlg.info.name || ""; font.pixelSize: Theme.fontSubtitle; font.weight: Font.DemiBold; elide: Text.ElideMiddle; Layout.maximumWidth: 380 }
                Label { text: dlg.info.type || ""; color: Theme.textSecondary }
            }
        }
        GridLayout {
            columns: 2
            columnSpacing: 18
            rowSpacing: 8
            Layout.fillWidth: true
            Repeater {
                model: [
                    [qsTr("Location"), dlg.info.location],
                    [qsTr("Size"), dlg.sizeText],
                    [qsTr("Kind"), dlg.info.mime],
                    [qsTr("Link to"), dlg.info.isLink ? dlg.info.linkTarget : ""],
                    [qsTr("Created"), dlg.info.created],
                    [qsTr("Modified"), dlg.info.modified],
                    [qsTr("Accessed"), dlg.info.accessed],
                    [qsTr("Owner"), (dlg.info.owner || "") + " / " + (dlg.info.group || "")],
                    [qsTr("Permissions"), dlg.info.permissions],
                    [qsTr("Access"), (dlg.info.readable ? qsTr("read") : "") + (dlg.info.writable ? qsTr(", write") : "")
                                     + (dlg.info.executable ? qsTr(", execute") : "")]
                ].filter(r => r[1] && String(r[1]).length)
                delegate: RowLayout {
                    required property var modelData
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    spacing: 18
                    Label { text: modelData[0]; color: Theme.textSecondary; Layout.preferredWidth: 100 }
                    Label {
                        text: modelData[1]
                        Layout.fillWidth: true
                        wrapMode: Text.WrapAnywhere
                        font.family: modelData[0] === qsTr("Permissions") ? Theme.monoFamily : Theme.fontFamily
                    }
                }
            }
        }
    }
}
