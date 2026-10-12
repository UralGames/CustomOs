import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Monitor

ApplicationWindow {
    id: win
    width: 1000
    height: 680
    minimumWidth: 700
    minimumHeight: 460
    visible: true
    title: tabs.currentIndex === 0 ? qsTr("Performance") : qsTr("Processes")

    SystemStats { id: stats; running: win.visible }
    ProcessModel { id: procs; running: win.visible && tabs.currentIndex === 1 }

    property int selectedPid: -1
    property string message: ""
    Timer { id: msgTimer; interval: 5000; onTriggered: win.message = "" }
    function endSelected(force) {
        if (selectedPid <= 0) return
        const err = procs.endProcess(selectedPid, force)
        message = err.length ? err : (force ? qsTr("Process %1 was killed.") : qsTr("Asked process %1 to end.")).arg(selectedPid)
        msgTimer.restart()
        if (!err.length) selectedPid = -1
    }

    Shortcut { sequences: [StandardKey.Quit, StandardKey.Close]; onActivated: Qt.quit() }
    Shortcut { sequence: "Delete"; enabled: tabs.currentIndex === 1; onActivated: win.endSelected(false) }
    Shortcut { sequences: [StandardKey.Find]; onActivated: { tabs.currentIndex = 1; search.forceActiveFocus() } }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            TabBar {
                id: tabs
                TabButton { text: qsTr("Performance"); width: implicitWidth }
                TabButton { text: qsTr("Processes"); width: implicitWidth }
            }
            Item { Layout.fillWidth: true }
            Label {
                text: qsTr("%1 processes • load %2").arg(stats.processCount).arg(stats.loadAverage)
                color: Theme.textSecondary
                font.pixelSize: Theme.fontCaption
                Layout.rightMargin: 8
            }
        }
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: tabs.currentIndex

        // ---------------- performance ----------------
        ScrollView {
            contentWidth: availableWidth
            contentHeight: perf.implicitHeight + 48
            clip: true
            GridLayout {
                id: perf
                x: 24
                y: 24
                width: parent.width - 48
                columns: width > 820 ? 2 : 1
                columnSpacing: 16
                rowSpacing: 16

                // CPU
                SectionCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignTop
                    RowLayout {
                        Layout.fillWidth: true
                        Icon { name: "cpu"; size: 18 }
                        Heading { text: qsTr("Processor"); level: 2; font.pixelSize: 15; Layout.fillWidth: true }
                        Label { text: Math.round(stats.cpu) + "%"; font.pixelSize: Theme.fontTitle; font.weight: Font.DemiBold; color: Theme.accent }
                    }
                    Label { text: SystemInfo.cpuModel; color: Theme.textSecondary; font.pixelSize: Theme.fontCaption; elide: Text.ElideRight; Layout.fillWidth: true }
                    Graph { Layout.fillWidth: true; Layout.preferredHeight: 120; values: stats.cpuHistory; lineColor: Theme.accent }
                    Flow {
                        Layout.fillWidth: true
                        spacing: 6
                        Repeater {
                            model: stats.perCore
                            delegate: ColumnLayout {
                                required property var modelData
                                required property int index
                                spacing: 2
                                Rectangle {
                                    width: 34; height: 30; radius: 4
                                    color: Theme.surfaceAlt
                                    Rectangle {
                                        anchors.bottom: parent.bottom
                                        width: parent.width; radius: 4
                                        height: parent.height * Math.min(1, modelData / 100)
                                        color: Theme.alpha(Theme.accent, 0.7)
                                    }
                                }
                                Label { text: qsTr("CPU %1").arg(index); font.pixelSize: 10; color: Theme.textTertiary; Layout.alignment: Qt.AlignHCenter }
                            }
                        }
                    }
                }

                // Memory
                SectionCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignTop
                    RowLayout {
                        Layout.fillWidth: true
                        Icon { name: "memory"; size: 18 }
                        Heading { text: qsTr("Memory"); level: 2; font.pixelSize: 15; Layout.fillWidth: true }
                        Label {
                            text: stats.memTotal > 0 ? Math.round(100 * stats.memUsed / stats.memTotal) + "%" : "—"
                            font.pixelSize: Theme.fontTitle; font.weight: Font.DemiBold; color: "#7D57A8"
                        }
                    }
                    Label {
                        text: qsTr("%1 in use of %2 • %3 cached").arg(Sys.formatBytes(stats.memUsed)).arg(Sys.formatBytes(stats.memTotal)).arg(Sys.formatBytes(stats.memCached))
                        color: Theme.textSecondary; font.pixelSize: Theme.fontCaption
                    }
                    Graph { Layout.fillWidth: true; Layout.preferredHeight: 120; values: stats.memHistory; lineColor: Theme.dark ? "#B79AD9" : "#7D57A8" }
                    Label {
                        visible: stats.swapTotal > 0
                        text: qsTr("Swap: %1 of %2").arg(Sys.formatBytes(stats.swapUsed)).arg(Sys.formatBytes(stats.swapTotal))
                        color: Theme.textSecondary; font.pixelSize: Theme.fontCaption
                    }
                }

                // Network
                SectionCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignTop
                    RowLayout {
                        Layout.fillWidth: true
                        Icon { name: "globe"; size: 18 }
                        Heading { text: qsTr("Network"); level: 2; font.pixelSize: 15; Layout.fillWidth: true }
                    }
                    Label {
                        text: qsTr("Receiving %1/s • Sending %2/s").arg(Sys.formatBytes(stats.netRx)).arg(Sys.formatBytes(stats.netTx))
                        color: Theme.textSecondary; font.pixelSize: Theme.fontCaption
                    }
                    Graph { Layout.fillWidth: true; Layout.preferredHeight: 120; values: stats.netHistory; maxValue: 0; lineColor: Theme.dark ? "#E8BB5E" : "#A57712" }
                }

                // Disks
                SectionCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignTop
                    RowLayout {
                        Layout.fillWidth: true
                        Icon { name: "harddisk"; size: 18 }
                        Heading { text: qsTr("Storage"); level: 2; font.pixelSize: 15; Layout.fillWidth: true }
                    }
                    Repeater {
                        model: stats.disks
                        delegate: ColumnLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: 4
                            RowLayout {
                                Layout.fillWidth: true
                                Label { text: modelData.mount; font.weight: Font.Medium; Layout.fillWidth: true }
                                Label { text: modelData.device + " • " + modelData.type; color: Theme.textTertiary; font.pixelSize: Theme.fontCaption }
                            }
                            ProgressBar { Layout.fillWidth: true; value: modelData.total > 0 ? 1 - modelData.free / modelData.total : 0 }
                            Label {
                                text: qsTr("%1 free of %2").arg(Sys.formatBytes(modelData.free)).arg(Sys.formatBytes(modelData.total))
                                color: Theme.textSecondary; font.pixelSize: Theme.fontCaption
                            }
                        }
                    }
                }
            }
        }

        // ---------------- processes ----------------
        ColumnLayout {
            spacing: 0
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 12
                spacing: 10
                TextField {
                    id: search
                    Layout.preferredWidth: 280
                    placeholderText: qsTr("Search by name, command or PID")
                    leftPadding: 34
                    onTextChanged: procs.filter = text
                    Icon { anchors.left: parent.left; anchors.leftMargin: 11; anchors.verticalCenter: parent.verticalCenter; name: "search"; size: 15; color: Theme.textSecondary }
                }
                CheckBox { text: qsTr("Only my processes"); checked: procs.onlyMine; onToggled: procs.onlyMine = checked }
                Item { Layout.fillWidth: true }
                Button { text: qsTr("End process"); enabled: win.selectedPid > 0; onClicked: win.endSelected(false) }
                Button { text: qsTr("Kill"); enabled: win.selectedPid > 0; onClicked: confirmKill.open() }
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: win.message.length ? 36 : 0
                visible: Layout.preferredHeight > 0
                color: Theme.accentSoft
                Label { anchors.verticalCenter: parent.verticalCenter; x: 16; text: win.message }
            }
            // header
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                color: Theme.window
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.divider }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 18
                    anchors.rightMargin: 18
                    spacing: 0
                    ColumnHeader { Layout.fillWidth: true; text: qsTr("Name"); key: "name"; model: procs }
                    ColumnHeader { Layout.preferredWidth: 80; text: qsTr("PID"); key: "pid"; model: procs }
                    ColumnHeader { Layout.preferredWidth: 110; text: qsTr("User"); key: "user"; model: procs }
                    ColumnHeader { Layout.preferredWidth: 80; text: qsTr("CPU"); key: "cpu"; model: procs; alignRight: true }
                    ColumnHeader { Layout.preferredWidth: 100; text: qsTr("Memory"); key: "memory"; model: procs; alignRight: true }
                }
            }
            ListView {
                id: plist
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: procs
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {}
                delegate: Item {
                    id: prow
                    required property int pid
                    required property string name
                    required property string command
                    required property string user
                    required property real cpu
                    required property real memory
                    required property bool mine
                    width: ListView.view.width
                    height: 34
                    readonly property bool selected: win.selectedPid === pid
                    Rectangle {
                        anchors.fill: parent
                        anchors.leftMargin: 6
                        anchors.rightMargin: 6
                        radius: Theme.radius
                        color: prow.selected ? Theme.accentSoft : rowMouse.containsMouse ? Theme.hover : "transparent"
                    }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 18
                        anchors.rightMargin: 18
                        spacing: 0
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0
                            Text { text: prow.name; color: Theme.text; font.pixelSize: 13; elide: Text.ElideRight; Layout.fillWidth: true }
                        }
                        Text { Layout.preferredWidth: 80; text: prow.pid; color: Theme.textSecondary; font.pixelSize: 13 }
                        Text { Layout.preferredWidth: 110; text: prow.user; color: prow.mine ? Theme.text : Theme.textSecondary; font.pixelSize: 13; elide: Text.ElideRight }
                        Text {
                            Layout.preferredWidth: 80
                            horizontalAlignment: Text.AlignRight
                            text: prow.cpu.toFixed(1) + "%"
                            color: prow.cpu > 50 ? Theme.danger : prow.cpu > 10 ? Theme.warning : Theme.textSecondary
                            font.pixelSize: 13
                        }
                        Text { Layout.preferredWidth: 100; horizontalAlignment: Text.AlignRight; text: Sys.formatBytes(prow.memory); color: Theme.textSecondary; font.pixelSize: 13 }
                    }
                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: (m) => {
                            win.selectedPid = prow.pid
                            if (m.button === Qt.RightButton) procMenu.popup()
                        }
                        ToolTip.visible: containsMouse && prow.command.length > 0
                        ToolTip.text: prow.command
                        ToolTip.delay: 1000
                    }
                }
            }
        }
    }

    Menu {
        id: procMenu
        MenuItem { text: qsTr("End process"); icon.name: "close"; shortcutText: "Delete"; onTriggered: win.endSelected(false) }
        MenuItem { text: qsTr("Kill"); icon.name: "warning"; iconColor: Theme.danger; textColor: Theme.danger; onTriggered: confirmKill.open() }
    }

    Dialog {
        id: confirmKill
        title: qsTr("Kill process %1?").arg(win.selectedPid)
        width: 420
        contentItem: Label {
            text: qsTr("The process will be stopped immediately and unsaved work in it will be lost. Try “End process” first.")
            wrapMode: Text.Wrap
            color: Theme.textSecondary
        }
        footer: DialogButtonBox {
            Button { text: qsTr("Cancel"); DialogButtonBox.buttonRole: DialogButtonBox.RejectRole }
            Button { text: qsTr("Kill"); DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole }
        }
        onAccepted: win.endSelected(true)
    }
}
