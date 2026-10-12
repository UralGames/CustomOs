import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Settings

SettingsPage {
    id: page
    title: qsTr("Personalisation")
    subtitle: qsTr("Theme, colour and background. Changes apply to the whole desktop immediately.")

    property var wallpapers: Sys.wallpapers()

    // ---- theme ----
    SectionCard {
        Layout.fillWidth: true
        Heading { text: qsTr("Theme"); level: 3 }
        RowLayout {
            spacing: 16
            Repeater {
                model: [{ dark: false, label: qsTr("Light") }, { dark: true, label: qsTr("Dark") }]
                delegate: ColumnLayout {
                    required property var modelData
                    spacing: 8
                    Rectangle {
                        id: preview
                        readonly property bool selected: Settings.darkMode === modelData.dark
                        width: 180
                        height: 112
                        radius: Theme.radiusLarge
                        color: modelData.dark ? "#1C2020" : "#E7ECEA"
                        border.width: selected ? 2 : 1
                        border.color: selected ? Theme.accent : Theme.border
                        // miniature window + taskbar
                        Rectangle {
                            x: 20; y: 16; width: 120; height: 62; radius: 6
                            color: modelData.dark ? "#2B3131" : "#FFFFFF"
                            Rectangle { x: 8; y: 8; width: 44; height: 6; radius: 3; color: modelData.dark ? "#4A5555" : "#D5DBD9" }
                            Rectangle { x: 8; y: 20; width: 84; height: 6; radius: 3; color: modelData.dark ? "#3A4343" : "#E6EAE8" }
                            Rectangle { x: 8; y: 32; width: 64; height: 6; radius: 3; color: modelData.dark ? "#3A4343" : "#E6EAE8" }
                            Rectangle { x: 8; y: 46; width: 30; height: 9; radius: 4; color: Theme.accentBase }
                        }
                        Rectangle {
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 1
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: parent.width - 2; height: 18
                            color: modelData.dark ? "#202525" : "#F5F7F6"
                            Row {
                                anchors.centerIn: parent
                                spacing: 6
                                Repeater { model: 4; Rectangle { width: 8; height: 8; radius: 2; color: index === 0 ? Theme.accentBase : (modelData.dark ? "#5A6565" : "#B9C2BF") } }
                            }
                        }
                        MouseArea { anchors.fill: parent; onClicked: Settings.darkMode = modelData.dark }
                    }
                    RadioButton {
                        text: modelData.label
                        checked: preview.selected
                        onClicked: Settings.darkMode = modelData.dark
                    }
                }
            }
        }
        Divider {}
        Heading { text: qsTr("Accent colour"); level: 3 }
        Flow {
            Layout.fillWidth: true
            spacing: 10
            Repeater {
                model: Theme.accentPresets
                delegate: Rectangle {
                    required property var modelData
                    width: 40; height: 40; radius: 20
                    color: modelData.color
                    border.width: Settings.accentColor.toLowerCase() === modelData.color.toLowerCase() ? 3 : 0
                    border.color: Theme.dark ? "#FFFFFF" : "#1A1E1D"
                    Icon {
                        anchors.centerIn: parent
                        visible: Settings.accentColor.toLowerCase() === modelData.color.toLowerCase()
                        name: "check"; size: 18; color: "#FFFFFF"
                    }
                    MouseArea {
                        id: sw
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: Settings.accentColor = modelData.color
                    }
                    ToolTip.visible: sw.containsMouse
                    ToolTip.text: modelData.name
                }
            }
        }
    }

    // ---- background ----
    SectionCard {
        Layout.fillWidth: true
        RowLayout {
            Layout.fillWidth: true
            Heading { text: qsTr("Background"); level: 3; Layout.fillWidth: true }
            Button {
                text: qsTr("Browse…")
                icon.name: "image"
                onClicked: chooser.openAt(Sys.standardDir("pictures"), "")
            }
        }
        GridLayout {
            Layout.fillWidth: true
            columns: Math.max(2, Math.floor(width / 188))
            columnSpacing: 12
            rowSpacing: 12
            Repeater {
                model: page.wallpapers
                delegate: Rectangle {
                    required property string modelData
                    readonly property bool selected: Settings.wallpaper === modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: width * 9 / 16
                    radius: Theme.radiusLarge
                    color: Theme.surfaceAlt
                    border.width: selected ? 3 : 0
                    border.color: Theme.accent
                    clip: true
                    Image {
                        anchors.fill: parent
                        anchors.margins: parent.selected ? 3 : 0
                        source: Sys.fileUrl(modelData)
                        sourceSize: Qt.size(320, 180)
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                    }
                    MouseArea { anchors.fill: parent; onClicked: Settings.wallpaper = modelData }
                }
            }
        }
        SettingRow {
            title: qsTr("Fit")
            description: qsTr("How the picture fills the screen")
            ComboBox {
                Layout.preferredWidth: 180
                readonly property var values: ["fill", "fit", "stretch", "center"]
                model: [qsTr("Fill"), qsTr("Fit"), qsTr("Stretch"), qsTr("Centre")]
                currentIndex: Math.max(0, values.indexOf(Settings.wallpaperFill))
                onActivated: (i) => Settings.wallpaperFill = values[i]
            }
        }
    }

    // ---- effects ----
    SectionCard {
        Layout.fillWidth: true
        Heading { text: qsTr("Effects"); level: 3 }
        SettingRow {
            title: qsTr("Transparency")
            description: qsTr("Let the wallpaper show slightly through the taskbar and panels")
            iconName: "eye"
            Switch { checked: Settings.transparency; onToggled: Settings.transparency = checked }
        }
        SettingRow {
            title: qsTr("Animations")
            description: qsTr("Fade and slide panels, menus and controls")
            iconName: "activity"
            Switch { checked: Settings.animations; onToggled: Settings.animations = checked }
        }
        SettingRow {
            title: qsTr("Rounded window corners")
            iconName: "maximize"
            Switch { checked: Settings.roundedCorners; onToggled: Settings.roundedCorners = checked }
        }
    }

    FileChooser {
        id: chooser
        title: qsTr("Choose a background picture")
        nameFilters: ["*.jpg", "*.jpeg", "*.png", "*.webp", "*.JPG", "*.PNG"]
        onAccepted: (path) => {
            const copy = Sys.importWallpaper(path)
            if (copy.length) {
                Settings.wallpaper = copy
                page.wallpapers = Sys.wallpapers()
            } else {
                page.notify(qsTr("Could not use that picture."), true)
            }
        }
    }
}
