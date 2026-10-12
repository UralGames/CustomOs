import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Files

ApplicationWindow {
    id: win
    property string startPath: Sys.homePath

    width: 1060
    height: 680
    minimumWidth: 640
    minimumHeight: 400
    visible: true
    title: dir.isTrash ? qsTr("Trash") : (dir.path === "/" ? qsTr("Computer") : Sys.fileName(dir.path))

    // ---- navigation ----------------------------------------------------------
    property var history: []
    property int historyIndex: -1
    property string viewMode: "list"
    property string banner: ""
    property bool bannerError: false

    function navigate(p, record) {
        if (!Sys.isDir(p)) {
            showBanner(qsTr("“%1” is not a folder that can be opened.").arg(p), true)
            return
        }
        dir.path = p
        clearSelection()
        if (record !== false) {
            history = history.slice(0, historyIndex + 1).concat([dir.path])
            historyIndex = history.length - 1
        }
        searchField.text = ""
        list.positionViewAtBeginning()
        grid.positionViewAtBeginning()
    }
    function goBack() { if (historyIndex > 0) { historyIndex--; navigate(history[historyIndex], false) } }
    function goForward() { if (historyIndex < history.length - 1) { historyIndex++; navigate(history[historyIndex], false) } }
    function goUp() { if (dir.path !== "/") navigate(Sys.parentDir(dir.path)) }

    function showBanner(text, isError) {
        banner = text
        bannerError = isError === true
        bannerTimer.restart()
    }
    Timer { id: bannerTimer; interval: 6000; onTriggered: win.banner = "" }

    // ---- selection -----------------------------------------------------------
    property var selection: ({})
    property int selVersion: 0
    property int anchorRow: -1
    property int currentRow: -1
    readonly property var selectedPaths: (selVersion, Object.keys(selection))

    function clearSelection() { selection = ({}); anchorRow = -1; currentRow = -1; selVersion++ }
    function isSelected(p) { return selection[p] === true }
    function selectOnly(row) {
        const s = {}
        const p = dir.pathAt(row)
        if (p) s[p] = true
        selection = s
        anchorRow = row
        currentRow = row
        selVersion++
    }
    function toggleRow(row) {
        const p = dir.pathAt(row)
        if (!p) return
        if (selection[p]) delete selection[p]
        else selection[p] = true
        anchorRow = row
        currentRow = row
        selVersion++
    }
    function selectRange(row) {
        if (anchorRow < 0) { selectOnly(row); return }
        const s = {}
        for (let i = Math.min(anchorRow, row); i <= Math.max(anchorRow, row); ++i) s[dir.pathAt(i)] = true
        selection = s
        currentRow = row
        selVersion++
    }
    function selectAll() {
        const s = {}
        for (let i = 0; i < dir.count; ++i) s[dir.pathAt(i)] = true
        selection = s
        selVersion++
    }
    function clickRow(row, modifiers) {
        if (modifiers & Qt.ControlModifier) toggleRow(row)
        else if (modifiers & Qt.ShiftModifier) selectRange(row)
        else selectOnly(row)
    }

    // ---- actions -------------------------------------------------------------
    function openPath(p) {
        if (Sys.isDir(p)) navigate(p)
        else if (!Apps.openFile(p)) {} // failure reported through onLaunchFailed
    }
    function openSelected() {
        const paths = selectedPaths
        if (paths.length === 1) openPath(paths[0])
        else paths.filter(p => !Sys.isDir(p)).forEach(p => Apps.openFile(p))
    }
    function deleteSelected(permanent) {
        const paths = selectedPaths
        if (!paths.length) return
        if (permanent || dir.isTrash) {
            confirmDialog.ask(paths.length === 1 ? qsTr("Delete “%1” permanently?").arg(Sys.fileName(paths[0]))
                                                 : qsTr("Delete %1 items permanently?").arg(paths.length),
                              qsTr("This cannot be undone."), qsTr("Delete"),
                              () => FileOps.deletePermanently(paths))
        } else {
            FileOps.trash(paths)
        }
        clearSelection()
    }
    function renameSelected() {
        if (selectedPaths.length !== 1) return
        nameDialog.start("rename", selectedPaths[0])
    }
    function copySelected(cut) {
        if (!selectedPaths.length) return
        FileOps.setClipboard(selectedPaths, cut)
        const n = selectedPaths.length
        const items = n === 1 ? qsTr("1 item") : qsTr("%1 items").arg(n)
        showBanner(cut ? qsTr("%1 ready to move — open a folder and paste").arg(items)
                       : qsTr("%1 copied").arg(items), false)
    }
    function paste() {
        if (dir.isTrash) return
        if (!FileOps.canPaste()) { showBanner(qsTr("Nothing to paste"), false); return }
        FileOps.paste(dir.path)
    }

    Connections {
        target: FileOps
        function onFinished(ok, message) { win.showBanner(message, !ok) }
    }
    Connections {
        target: Apps
        function onLaunchFailed(name, reason) { win.showBanner(reason, true) }
    }

    DirModel {
        id: dir
        showHidden: false
        onPathChanged: win.clearSelection()
    }

    Component.onCompleted: navigate(startPath)

    // ---- keyboard ------------------------------------------------------------
    Shortcut { sequences: [StandardKey.Back, "Alt+Left", "Backspace"]; onActivated: win.goBack() }
    Shortcut { sequences: [StandardKey.Forward, "Alt+Right"]; onActivated: win.goForward() }
    Shortcut { sequence: "Alt+Up"; onActivated: win.goUp() }
    Shortcut { sequence: "Alt+Home"; onActivated: win.navigate(Sys.homePath) }
    Shortcut { sequences: [StandardKey.Refresh]; onActivated: dir.reload() }
    Shortcut { sequences: [StandardKey.SelectAll]; onActivated: win.selectAll() }
    Shortcut { sequences: [StandardKey.Copy]; onActivated: win.copySelected(false) }
    Shortcut { sequences: [StandardKey.Cut]; onActivated: win.copySelected(true) }
    Shortcut { sequences: [StandardKey.Paste]; onActivated: win.paste() }
    Shortcut { sequences: [StandardKey.Delete]; onActivated: win.deleteSelected(false) }
    Shortcut { sequence: "Shift+Delete"; onActivated: win.deleteSelected(true) }
    Shortcut { sequence: "F2"; onActivated: win.renameSelected() }
    Shortcut { sequence: "Ctrl+Shift+N"; onActivated: nameDialog.start("folder", dir.path) }
    Shortcut { sequence: "Ctrl+H"; onActivated: dir.showHidden = !dir.showHidden }
    Shortcut { sequences: ["Ctrl+L", "F6"]; onActivated: pathBar.editing = true }
    Shortcut { sequences: [StandardKey.Find]; onActivated: searchField.forceActiveFocus() }
    Shortcut { sequence: "Return"; enabled: !nameDialog.visible && !pathBar.editing && !searchField.activeFocus; onActivated: win.openSelected() }
    Shortcut { sequence: "Alt+Return"; onActivated: if (win.selectedPaths.length === 1) propsDialog.show(win.selectedPaths[0]) }
    Shortcut { sequence: "Ctrl+1"; onActivated: win.viewMode = "list" }
    Shortcut { sequence: "Ctrl+2"; onActivated: win.viewMode = "grid" }
    Shortcut { sequence: "Escape"; onActivated: win.clearSelection() }
    Shortcut { sequences: [StandardKey.Quit]; onActivated: Qt.quit() }

    // ---- toolbar -----------------------------------------------------------
    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: 4
            ToolButton { icon.name: "arrow-left"; enabled: win.historyIndex > 0; onClicked: win.goBack()
                ToolTip.visible: hovered; ToolTip.text: qsTr("Back (Alt+Left)") }
            ToolButton { icon.name: "arrow-right"; enabled: win.historyIndex < win.history.length - 1; onClicked: win.goForward()
                ToolTip.visible: hovered; ToolTip.text: qsTr("Forward (Alt+Right)") }
            ToolButton { icon.name: "arrow-up"; enabled: dir.path !== "/"; onClicked: win.goUp()
                ToolTip.visible: hovered; ToolTip.text: qsTr("Up (Alt+Up)") }
            ToolButton { icon.name: "refresh"; onClicked: dir.reload()
                ToolTip.visible: hovered; ToolTip.text: qsTr("Refresh (F5)") }

            PathBar {
                id: pathBar
                Layout.fillWidth: true
                Layout.leftMargin: 6
                Layout.rightMargin: 6
                path: dir.path
                onNavigateRequested: (p) => win.navigate(p)
            }

            TextField {
                id: searchField
                Layout.preferredWidth: 220
                placeholderText: qsTr("Filter this folder")
                leftPadding: 34
                onTextChanged: dir.filter = text
                Keys.onEscapePressed: { text = ""; list.forceActiveFocus() }
                Icon { anchors.left: parent.left; anchors.leftMargin: 11; anchors.verticalCenter: parent.verticalCenter
                       name: "search"; size: 15; color: Theme.textSecondary }
            }
            ToolSeparator {}
            ToolButton {
                icon.name: "folder-plus"
                enabled: !dir.isTrash
                onClicked: nameDialog.start("folder", dir.path)
                ToolTip.visible: hovered; ToolTip.text: qsTr("New folder (Ctrl+Shift+N)")
            }
            ToolButton {
                icon.name: win.viewMode === "list" ? "grid" : "list"
                onClicked: win.viewMode = win.viewMode === "list" ? "grid" : "list"
                ToolTip.visible: hovered; ToolTip.text: win.viewMode === "list" ? qsTr("Icon view") : qsTr("Details view")
            }
            ToolButton {
                id: moreButton
                icon.name: "dots"
                onClicked: moreMenu.popup(moreButton, 0, moreButton.height)
            }
        }
    }

    Menu {
        id: moreMenu
        MenuItem { text: qsTr("Show hidden files"); checkable: true; checked: dir.showHidden; shortcutText: "Ctrl+H"
                   onTriggered: dir.showHidden = !dir.showHidden }
        MenuSeparator {}
        MenuItem { text: qsTr("Sort by name"); checkable: true; checked: dir.sortKey === "name"; onTriggered: dir.sortKey = "name" }
        MenuItem { text: qsTr("Sort by date modified"); checkable: true; checked: dir.sortKey === "modified"; onTriggered: dir.sortKey = "modified" }
        MenuItem { text: qsTr("Sort by type"); checkable: true; checked: dir.sortKey === "type"; onTriggered: dir.sortKey = "type" }
        MenuItem { text: qsTr("Sort by size"); checkable: true; checked: dir.sortKey === "size"; onTriggered: dir.sortKey = "size" }
        MenuItem { text: qsTr("Descending"); checkable: true; checked: dir.sortDescending; onTriggered: dir.sortDescending = !dir.sortDescending }
        MenuSeparator {}
        MenuItem { text: qsTr("Open terminal here"); icon.name: "terminal"; onTriggered: Apps.openTerminal(dir.path) }
        MenuItem { text: qsTr("Empty Trash"); icon.name: "trash"; enabled: true
                   onTriggered: confirmDialog.ask(qsTr("Empty the Trash?"), qsTr("All items in the Trash will be deleted permanently."),
                                                  qsTr("Empty Trash"), () => FileOps.emptyTrash()) }
    }

    // ---- body ----------------------------------------------------------------
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Sidebar
        Rectangle {
            Layout.preferredWidth: 220
            Layout.fillHeight: true
            color: Theme.sidebar
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 2
                Repeater {
                    model: [
                        { label: qsTr("Home"), icon: "home", path: Sys.homePath },
                        { label: qsTr("Desktop"), icon: "desktop", path: Sys.standardDir("desktop") },
                        { label: qsTr("Documents"), icon: "documents", path: Sys.standardDir("documents") },
                        { label: qsTr("Downloads"), icon: "downloads", path: Sys.standardDir("downloads") },
                        { label: qsTr("Music"), icon: "music", path: Sys.standardDir("music") },
                        { label: qsTr("Pictures"), icon: "pictures", path: Sys.standardDir("pictures") },
                        { label: qsTr("Videos"), icon: "videos", path: Sys.standardDir("videos") }
                    ]
                    delegate: ItemDelegate {
                        required property var modelData
                        Layout.fillWidth: true
                        visible: Sys.isDir(modelData.path)
                        text: modelData.label
                        icon.name: modelData.icon
                        highlighted: dir.path === modelData.path
                        onClicked: win.navigate(modelData.path)
                    }
                }
                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; Layout.margins: 6; color: Theme.divider }
                ItemDelegate {
                    Layout.fillWidth: true
                    text: qsTr("Trash")
                    icon.name: "trash"
                    highlighted: dir.isTrash
                    onClicked: { FileOps.ensureTrash(); win.navigate(FileOps.trashPath) }
                }
                ItemDelegate {
                    Layout.fillWidth: true
                    text: qsTr("Computer")
                    icon.name: "drive"
                    highlighted: dir.path === "/"
                    onClicked: win.navigate("/")
                }
                Item { Layout.fillHeight: true }
                // Disk space of the current location
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.margins: 8
                    spacing: 6
                    readonly property var usage: (dir.count, SystemInfo.diskUsage(dir.path || "/"))
                    Label {
                        text: qsTr("%1 free of %2").arg(Sys.formatBytes(parent.usage.free)).arg(Sys.formatBytes(parent.usage.total))
                        font.pixelSize: Theme.fontCaption
                        color: Theme.textSecondary
                    }
                    ProgressBar {
                        Layout.fillWidth: true
                        value: parent.usage.total > 0 ? 1 - parent.usage.free / parent.usage.total : 0
                    }
                }
            }
        }

        // Main area
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Banner (results and errors)
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: win.banner.length ? 40 : 0
                visible: Layout.preferredHeight > 0
                color: win.bannerError ? Theme.dangerBackground : Theme.accentSoft
                Behavior on Layout.preferredHeight { NumberAnimation { duration: Theme.duration } }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 8
                    Icon { name: win.bannerError ? "warning" : "info"; size: 16; color: win.bannerError ? Theme.danger : Theme.accent }
                    Label { Layout.fillWidth: true; text: win.banner; elide: Text.ElideRight }
                    ToolButton { icon.name: "close"; onClicked: win.banner = "" }
                }
            }

            // Trash actions
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: dir.isTrash ? 48 : 0
                visible: dir.isTrash
                color: Theme.surfaceAlt
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 12
                    Label { Layout.fillWidth: true; text: qsTr("Items in the Trash can be restored to where they were deleted from."); color: Theme.textSecondary }
                    Button { text: qsTr("Restore"); enabled: win.selectedPaths.length > 0
                             onClicked: { FileOps.restore(win.selectedPaths); win.clearSelection() } }
                    Button { text: qsTr("Empty Trash"); enabled: dir.count > 0
                             onClicked: confirmDialog.ask(qsTr("Empty the Trash?"), qsTr("All items in the Trash will be deleted permanently."),
                                                          qsTr("Empty Trash"), () => FileOps.emptyTrash()) }
                }
            }

            // Column headers (details view)
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                visible: win.viewMode === "list"
                color: Theme.window
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.divider }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 0
                    SortHeader { Layout.fillWidth: true; text: qsTr("Name"); key: "name"; model: dir }
                    SortHeader { Layout.preferredWidth: 170; text: qsTr("Date modified"); key: "modified"; model: dir }
                    SortHeader { Layout.preferredWidth: 160; text: qsTr("Type"); key: "type"; model: dir }
                    SortHeader { Layout.preferredWidth: 90; text: qsTr("Size"); key: "size"; model: dir; alignRight: true }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                // Empty-area clicks and context menu
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onPressed: (m) => {
                        win.clearSelection()
                        list.forceActiveFocus()
                        if (m.button === Qt.RightButton) areaMenu.popup()
                    }
                }

                ListView {
                    id: list
                    anchors.fill: parent
                    anchors.margins: 6
                    visible: win.viewMode === "list"
                    model: dir
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    keyNavigationEnabled: false
                    ScrollBar.vertical: ScrollBar {}
                    delegate: FileRow {
                        width: ListView.view.width
                        selected: (win.selVersion, win.isSelected(filePath))
                        isCut: FileOps.clipboardCut && FileOps.clipboardPaths.indexOf(filePath) >= 0
                        onClicked: (mods) => win.clickRow(index, mods)
                        onDoubleClicked: win.openPath(filePath)
                        onContextRequested: { if (!win.isSelected(filePath)) win.selectOnly(index); itemMenu.popup() }
                    }
                }

                GridView {
                    id: grid
                    anchors.fill: parent
                    anchors.margins: 10
                    visible: win.viewMode === "grid"
                    model: dir
                    clip: true
                    cellWidth: 120
                    cellHeight: 128
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar {}
                    delegate: FileTile {
                        width: grid.cellWidth
                        height: grid.cellHeight
                        selected: (win.selVersion, win.isSelected(filePath))
                        isCut: FileOps.clipboardCut && FileOps.clipboardPaths.indexOf(filePath) >= 0
                        onClicked: (mods) => win.clickRow(index, mods)
                        onDoubleClicked: win.openPath(filePath)
                        onContextRequested: { if (!win.isSelected(filePath)) win.selectOnly(index); itemMenu.popup() }
                    }
                }

                ColumnLayout {
                    anchors.centerIn: parent
                    visible: dir.count === 0
                    spacing: 10
                    Icon { Layout.alignment: Qt.AlignHCenter; name: dir.error.length ? "warning" : dir.isTrash ? "trash" : "folder"; size: 40; color: Theme.textTertiary }
                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: dir.error.length ? dir.error
                              : dir.filter.length ? qsTr("No items match “%1”").arg(dir.filter)
                              : dir.isTrash ? qsTr("The Trash is empty") : qsTr("This folder is empty")
                        color: Theme.textSecondary
                    }
                }
            }

            // Status bar
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                color: Theme.window
                Rectangle { width: parent.width; height: 1; color: Theme.divider }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    Label {
                        text: win.selectedPaths.length > 0
                              ? qsTr("%1 of %2 items selected").arg(win.selectedPaths.length).arg(dir.count)
                                + (dir.totalSize(win.selectedPaths) > 0 ? " • " + Sys.formatBytes(dir.totalSize(win.selectedPaths)) : "")
                              : (dir.count === 1 ? qsTr("1 item") : qsTr("%1 items").arg(dir.count))
                        font.pixelSize: Theme.fontCaption
                        color: Theme.textSecondary
                    }
                    Item { Layout.fillWidth: true }
                    Label { visible: FileOps.busy; text: FileOps.status; font.pixelSize: Theme.fontCaption; color: Theme.textSecondary }
                    ProgressBar { visible: FileOps.busy; Layout.preferredWidth: 140; value: FileOps.progress }
                }
            }
        }
    }

    // ---- menus -----------------------------------------------------------------
    Menu {
        id: itemMenu
        readonly property int n: win.selectedPaths.length
        readonly property string first: n > 0 ? win.selectedPaths[0] : ""
        MenuItem { text: qsTr("Open"); icon.name: "open"; shortcutText: "Enter"; onTriggered: win.openSelected() }
        MenuItem {
            text: qsTr("Open with…")
            icon.name: "apps"
            enabled: itemMenu.n === 1 && !Sys.isDir(itemMenu.first)
            onTriggered: openWithDialog.show(itemMenu.first)
        }
        MenuItem { text: qsTr("Open terminal here"); icon.name: "terminal"; enabled: itemMenu.n === 1 && Sys.isDir(itemMenu.first)
                   onTriggered: Apps.openTerminal(itemMenu.first) }
        MenuSeparator {}
        MenuItem { text: qsTr("Restore"); icon.name: "undo"; visible: dir.isTrash; height: visible ? implicitHeight : 0
                   onTriggered: { FileOps.restore(win.selectedPaths); win.clearSelection() } }
        MenuItem { text: qsTr("Cut"); icon.name: "cut"; shortcutText: "Ctrl+X"; enabled: !dir.isTrash; onTriggered: win.copySelected(true) }
        MenuItem { text: qsTr("Copy"); icon.name: "copy"; shortcutText: "Ctrl+C"; onTriggered: win.copySelected(false) }
        MenuItem { text: qsTr("Rename…"); icon.name: "rename"; shortcutText: "F2"; enabled: itemMenu.n === 1 && !dir.isTrash
                   onTriggered: win.renameSelected() }
        MenuSeparator {}
        MenuItem { text: qsTr("Move to Trash"); icon.name: "trash"; shortcutText: "Delete"; visible: !dir.isTrash; height: visible ? implicitHeight : 0
                   onTriggered: win.deleteSelected(false) }
        MenuItem { text: qsTr("Delete permanently"); icon.name: "close"; shortcutText: "Shift+Delete"
                   iconColor: Theme.danger; textColor: Theme.danger; onTriggered: win.deleteSelected(true) }
        MenuSeparator {}
        MenuItem { text: qsTr("Properties"); icon.name: "properties"; shortcutText: "Alt+Enter"; enabled: itemMenu.n === 1
                   onTriggered: propsDialog.show(itemMenu.first) }
    }

    Menu {
        id: areaMenu
        MenuItem { text: qsTr("New folder"); icon.name: "folder-plus"; enabled: !dir.isTrash; onTriggered: nameDialog.start("folder", dir.path) }
        MenuItem { text: qsTr("New text document"); icon.name: "file-plus"; enabled: !dir.isTrash; onTriggered: nameDialog.start("file", dir.path) }
        MenuItem { text: qsTr("Paste"); icon.name: "paste"; shortcutText: "Ctrl+V"; enabled: !dir.isTrash && FileOps.clipboardPaths.length > 0
                   onTriggered: win.paste() }
        MenuSeparator {}
        MenuItem { text: qsTr("Select all"); shortcutText: "Ctrl+A"; onTriggered: win.selectAll() }
        MenuItem { text: qsTr("Open terminal here"); icon.name: "terminal"; onTriggered: Apps.openTerminal(dir.path) }
        MenuItem { text: qsTr("Properties"); icon.name: "properties"; onTriggered: propsDialog.show(dir.path) }
    }

    // ---- dialogs ---------------------------------------------------------------
    NameDialog {
        id: nameDialog
        onDone: (error) => { if (error.length) win.showBanner(error, true) }
    }
    PropertiesDialog { id: propsDialog }
    OpenWithDialog { id: openWithDialog }

    Dialog {
        id: confirmDialog
        property var action: null
        property string detail: ""
        property string acceptText: qsTr("OK")
        function ask(titleText, detailText, okText, fn) {
            title = titleText
            detail = detailText
            acceptText = okText
            action = fn
            open()
        }
        width: 420
        contentItem: Label { text: confirmDialog.detail; wrapMode: Text.Wrap; color: Theme.textSecondary }
        footer: DialogButtonBox {
            Button { text: qsTr("Cancel"); DialogButtonBox.buttonRole: DialogButtonBox.RejectRole }
            Button { text: confirmDialog.acceptText; DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole }
        }
        onAccepted: if (action) action()
    }
}
