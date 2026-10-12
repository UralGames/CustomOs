import QtQuick
import Meridian.Core
import Meridian.Shell

// Desktop background with a cross-fade when the wallpaper changes.
Item {
    id: root
    readonly property string source: Settings.wallpaper
    readonly property int fill: {
        switch (Settings.wallpaperFill) {
        case "fit": return Image.PreserveAspectFit
        case "stretch": return Image.Stretch
        case "center": return Image.Pad
        default: return Image.PreserveAspectCrop
        }
    }
    property bool showA: true

    Rectangle {
        anchors.fill: parent
        color: Theme.dark ? "#101615" : "#D9E3DF"
    }

    Image {
        id: a
        anchors.fill: parent
        fillMode: root.fill
        sourceSize: Qt.size(root.width, root.height)
        asynchronous: true
        cache: false
        opacity: root.showA ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.durationSlow * 2 } }
    }
    Image {
        id: b
        anchors.fill: parent
        fillMode: root.fill
        sourceSize: Qt.size(root.width, root.height)
        asynchronous: true
        cache: false
        opacity: root.showA ? 0 : 1
        Behavior on opacity { NumberAnimation { duration: Theme.durationSlow * 2 } }
    }

    function apply() {
        const url = source.length ? Sys.fileUrl(source) : ""
        if (showA) {
            if (a.source == url) return
            b.source = url
        } else {
            if (b.source == url) return
            a.source = url
        }
        showA = !showA
    }
    onSourceChanged: apply()
    Component.onCompleted: a.source = source.length ? Sys.fileUrl(source) : ""
}
