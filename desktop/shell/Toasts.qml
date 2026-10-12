import QtQuick
import Meridian.Core
import Meridian.Shell

// Stack of pop-up notifications (newest at the bottom, like a feed).
Item {
    id: toasts
    property bool suppressed: false
    property var payloads: ({})      // notification id -> data
    readonly property int count: toastModel.count
    signal layoutChanged()

    width: 380
    height: column.height

    ListModel { id: toastModel }

    function removeId(id) {
        for (let i = 0; i < toastModel.count; ++i) {
            if (toastModel.get(i).nid === id) {
                toastModel.remove(i)
                break
            }
        }
        delete payloads[id]
    }
    // Rectangles (window coordinates) that should receive clicks.
    function cardRects() {
        const rects = []
        for (let i = 0; i < repeater.count; ++i) {
            const it = repeater.itemAt(i)
            if (!it) continue
            const p = it.mapToItem(null, 0, 0)
            rects.push(Qt.rect(p.x, p.y, it.width, it.height))
        }
        return rects
    }

    Connections {
        target: Notifications
        function onToast(n) {
            toasts.removeId(n.notificationId)
            toasts.payloads[n.notificationId] = n
            toastModel.append({ nid: n.notificationId, critical: n.urgency >= 2 })
            while (toastModel.count > 4)
                toasts.removeId(toastModel.get(0).nid)
        }
        function onNotificationClosed(id, reason) { toasts.removeId(id) }
    }
    onSuppressedChanged: if (suppressed) { toastModel.clear(); payloads = ({}) }

    Column {
        id: column
        width: parent.width
        anchors.bottom: parent.bottom
        spacing: 10
        onHeightChanged: toasts.layoutChanged()
        move: Transition {
            NumberAnimation { property: "y"; duration: Theme.duration; easing.type: Theme.easing }
        }

        Repeater {
            id: repeater
            model: toastModel
            onCountChanged: toasts.layoutChanged()
            delegate: Item {
                id: wrap
                required property int nid
                required property bool critical
                readonly property var payload: toasts.payloads[nid] || ({})
                width: column.width
                height: card.implicitHeight

                BorderImage {
                    anchors.fill: parent
                    anchors.margins: -20
                    anchors.bottomMargin: -26
                    source: "qrc:/images/shadow-popup.png"
                    border { left: 46; right: 46; top: 46; bottom: 46 }
                    opacity: card.opacity * 0.7
                }
                NotificationCard {
                    id: card
                    width: parent.width
                    height: parent.height
                    toast: true
                    data_: wrap.payload
                    onDismissed: Notifications.dismiss(wrap.nid)
                    onActionClicked: (key) => Notifications.invokeAction(wrap.nid, key)
                    onClicked: {
                        if (wrap.payload.hasDefault) Notifications.invokeAction(wrap.nid, "default")
                        toasts.removeId(wrap.nid)
                    }
                    opacity: 0
                    x: 40
                    Component.onCompleted: { opacity = 1; x = 0 }
                    Behavior on opacity { NumberAnimation { duration: Theme.duration } }
                    Behavior on x { NumberAnimation { duration: Theme.duration; easing.type: Theme.easing } }
                    HoverHandler { id: hov }
                }
                Timer {
                    interval: 6000
                    running: !wrap.critical && !hov.hovered
                    onTriggered: {
                        Notifications.expire(wrap.nid)
                        toasts.removeId(wrap.nid)
                    }
                }
            }
        }
    }
}
