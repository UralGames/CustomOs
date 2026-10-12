import QtQuick
import Meridian.Core

// Icon for a file or folder. Folders use the coloured folder artwork;
// files use a symbolic glyph on a soft tile tinted by type. Images show a
// thumbnail when `thumbnail` is set.
Item {
    id: fi
    property string category: "other"
    property string iconName: "file"
    property string path: ""
    property int size: 20
    property bool thumbnail: false

    readonly property color tint: {
        switch (category) {
        case "image": return "#3B82C4"
        case "audio": return "#B0508F"
        case "video": return "#7D57B8"
        case "archive": return "#A57712"
        case "code": return "#1F8A7A"
        case "text": return "#5F6E78"
        case "exec": return "#4A5560"
        case "document": return "#C2372F"
        default: return "#6F7B83"
        }
    }

    width: size
    height: size

    AppIcon {
        anchors.fill: parent
        visible: fi.category === "folder"
        name: "meridian-files"
        size: fi.size
    }
    Rectangle {
        anchors.fill: parent
        visible: fi.category !== "folder" && !thumb.visible
        radius: Math.max(3, fi.size / 5)
        color: Theme.alpha(fi.tint, Theme.dark ? 0.28 : 0.14)
        Icon {
            anchors.centerIn: parent
            name: fi.iconName
            size: Math.round(fi.size * 0.62)
            color: Theme.dark ? Qt.lighter(fi.tint, 1.6) : fi.tint
        }
    }
    Image {
        id: thumb
        anchors.fill: parent
        visible: fi.thumbnail && fi.category === "image" && status === Image.Ready
        source: fi.thumbnail && fi.category === "image" ? Sys.fileUrl(fi.path) : ""
        sourceSize: Qt.size(fi.size * 2, fi.size * 2)
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        cache: true
    }
}
