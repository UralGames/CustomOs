import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core

// Choose an application to open a file with (and optionally make it the default).
Dialog {
    id: dlg
    property string path: ""
    property string mime: ""
    property var suggested: []
    property var others: []
    property string chosen: ""

    function show(p) {
        path = p
        mime = Apps.mimeTypeFor(p)
        suggested = Apps.appsForFile(p)
        const ids = suggested.map(a => a.appId)
        others = Apps.all().filter(a => ids.indexOf(a.appId) < 0)
        chosen = suggested.length ? suggested[0].appId : ""
        always.checked = false
        open()
    }

    title: qsTr("Open “%1” with").arg(Sys.fileName(path))
    width: 480
    height: 520

    contentItem: ColumnLayout {
        spacing: 8
        Label {
            text: qsTr("File type: %1").arg(Apps.mimeDescription(dlg.path) || dlg.mime)
            color: Theme.textSecondary
        }
        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}
            model: dlg.suggested.concat([{ header: true }]).concat(dlg.others)
            delegate: Loader {
                required property var modelData
                width: ListView.view.width
                sourceComponent: modelData.header ? headerComp : appComp
                Component {
                    id: headerComp
                    Label {
                        topPadding: 12
                        bottomPadding: 4
                        leftPadding: 8
                        text: qsTr("Other applications")
                        font.pixelSize: Theme.fontCaption
                        font.weight: Font.DemiBold
                        color: Theme.textSecondary
                        visible: dlg.others.length > 0
                        height: visible ? implicitHeight : 0
                    }
                }
                Component {
                    id: appComp
                    ItemDelegate {
                        highlighted: dlg.chosen === modelData.appId
                        contentItem: Row {
                            spacing: 12
                            AppIcon { anchors.verticalCenter: parent.verticalCenter; name: modelData.iconName; size: 24 }
                            Text { anchors.verticalCenter: parent.verticalCenter; text: modelData.name; color: Theme.text; font.pixelSize: 14 }
                        }
                        onClicked: dlg.chosen = modelData.appId
                        onDoubleClicked: { dlg.chosen = modelData.appId; dlg.accept() }
                    }
                }
            }
        }
        CheckBox {
            id: always
            text: qsTr("Always use this app for %1 files").arg(Apps.mimeDescription(dlg.path) || dlg.mime)
        }
    }
    footer: DialogButtonBox {
        Button { text: qsTr("Cancel"); DialogButtonBox.buttonRole: DialogButtonBox.RejectRole }
        Button { text: qsTr("Open"); enabled: dlg.chosen.length > 0; DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole }
    }
    onAccepted: {
        if (!chosen.length) return
        if (always.checked) Apps.setDefaultApp(mime, chosen)
        Apps.openFileWith(chosen, path)
    }
}
