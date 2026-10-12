import QtQuick
import QtQuick.Templates as T
import Meridian.Core

T.Label {
    color: enabled ? Theme.text : Theme.textDisabled
    linkColor: Theme.accent
    font.pixelSize: Theme.fontBody
}
