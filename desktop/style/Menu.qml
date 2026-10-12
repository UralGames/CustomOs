import QtQuick
import QtQuick.Templates as T
import Meridian.Core

T.Menu {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)
    margins: 6
    padding: 4
    overlap: 4

    delegate: MenuItem {}

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.durationFast }
        NumberAnimation { property: "scale"; from: 0.96; to: 1; duration: Theme.duration; easing.type: Theme.easing }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.durationFast }
    }

    contentItem: ListView {
        implicitHeight: contentHeight
        model: control.contentModel
        interactive: Window.window ? contentHeight + control.topPadding + control.bottomPadding > Window.window.height : false
        clip: true
        currentIndex: control.currentIndex
        ScrollBar.vertical: ScrollBar {}
    }

    background: Rectangle {
        implicitWidth: 220
        implicitHeight: 32
        color: Theme.panelSolid
        radius: Theme.radiusLarge
        border.color: Theme.border
        border.width: 1
        PopupShadow { spread: 24 }
    }

    T.Overlay.modal: Rectangle { color: "transparent" }
    T.Overlay.modeless: Item {}
}
