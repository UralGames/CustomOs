import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core

// Breadcrumb path bar; click empty space (or Ctrl+L) to type a path.
Item {
    id: bar
    property string path: "/"
    property bool editing: false
    signal navigateRequested(string path)

    implicitHeight: 34

    readonly property var crumbs: {
        const home = Sys.homePath
        let base = "", parts = []
        let p = path
        if (p === home || p.startsWith(home + "/")) {
            parts.push({ label: qsTr("Home"), path: home })
            base = home
            p = p.substring(home.length)
        } else {
            parts.push({ label: qsTr("Computer"), path: "/" })
        }
        let acc = base
        for (const seg of p.split("/")) {
            if (!seg.length) continue
            acc = acc + "/" + seg
            parts.push({ label: seg, path: acc })
        }
        return parts
    }

    onEditingChanged: if (editing) { field.text = path; field.forceActiveFocus(); field.selectAll() }

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: Theme.inputBackground
        border.color: bar.editing ? Theme.accent : Theme.controlBorder
        visible: !bar.editing
        MouseArea { anchors.fill: parent; onClicked: bar.editing = true }

        ListView {
            id: crumbList
            anchors.fill: parent
            anchors.leftMargin: 4
            anchors.rightMargin: 4
            orientation: ListView.Horizontal
            clip: true
            interactive: false
            model: bar.crumbs
            onCountChanged: positionViewAtEnd()
            delegate: Row {
                required property var modelData
                required property int index
                height: crumbList.height
                Icon {
                    visible: index > 0
                    anchors.verticalCenter: parent.verticalCenter
                    name: "chevron-right"
                    size: 12
                    color: Theme.textTertiary
                }
                ToolButton {
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData.label
                    icon.name: index === 0 ? (modelData.path === "/" ? "drive" : "home") : ""
                    implicitHeight: 28
                    font.weight: index === bar.crumbs.length - 1 ? Font.DemiBold : Font.Normal
                    onClicked: bar.navigateRequested(modelData.path)
                }
            }
        }
    }

    TextField {
        id: field
        anchors.fill: parent
        visible: bar.editing
        onAccepted: { bar.editing = false; bar.navigateRequested(text.trim()) }
        onActiveFocusChanged: if (!activeFocus) bar.editing = false
        Keys.onEscapePressed: bar.editing = false
    }
}
