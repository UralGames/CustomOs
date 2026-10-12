import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Files

// Asks for a name: new folder, new file, or rename.
Dialog {
    id: dlg
    property string mode: "folder"    // folder | file | rename
    property string target: ""        // directory (new) or file (rename)
    property string error: ""
    signal done(string error)

    function start(m, t) {
        mode = m
        target = t
        error = ""
        if (m === "rename") {
            field.text = Sys.fileName(t)
            title = qsTr("Rename")
        } else if (m === "file") {
            field.text = FileOps.uniqueName(t, qsTr("New document.txt"))
            title = qsTr("New text document")
        } else {
            field.text = FileOps.uniqueName(t, qsTr("New folder"))
            title = qsTr("New folder")
        }
        open()
    }

    function apply() {
        const name = field.text
        let err
        if (mode === "rename") err = FileOps.rename(target, name)
        else if (mode === "file") err = FileOps.newFile(target, name)
        else err = FileOps.newFolder(target, name)
        if (err.length) { error = err; return }
        close()
        done("")
    }

    width: 420
    onOpened: {
        field.forceActiveFocus()
        // Select the name without its extension, like other desktops do.
        const dot = field.text.lastIndexOf(".")
        if (mode !== "folder" && dot > 0 && !Sys.isDir(target + "/" + field.text)) field.select(0, dot)
        else field.selectAll()
    }

    contentItem: ColumnLayout {
        spacing: 10
        TextField {
            id: field
            Layout.fillWidth: true
            onAccepted: dlg.apply()
            onTextChanged: dlg.error = ""
        }
        Label {
            Layout.fillWidth: true
            visible: dlg.error.length > 0
            text: dlg.error
            color: Theme.danger
            wrapMode: Text.Wrap
        }
    }
    footer: DialogButtonBox {
        Button { text: qsTr("Cancel"); DialogButtonBox.buttonRole: DialogButtonBox.RejectRole }
        Button {
            text: dlg.mode === "rename" ? qsTr("Rename") : qsTr("Create")
            DialogButtonBox.buttonRole: DialogButtonBox.ApplyRole
            onClicked: dlg.apply()
        }
    }
}
