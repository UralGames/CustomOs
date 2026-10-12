import QtQuick

// Soft drop shadow drawn from a pre-blurred 9-patch image (no runtime blur).
BorderImage {
    property real spread: 32
    anchors.fill: parent
    anchors.margins: -spread
    anchors.topMargin: -spread + 4
    anchors.bottomMargin: -spread - 6
    source: "qrc:/images/shadow-popup.png"
    border { left: 46; right: 46; top: 46; bottom: 46 }
    z: -1
}
