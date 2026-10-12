import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Editor

ApplicationWindow {
    id: win
    property string startFile: ""

    property string filePath: ""
    property string savedText: ""
    property string lineEnding: "LF"
    property string encoding: "UTF-8"
    property bool readOnly: false
    property bool wrap: true
    property bool mono: true
    property int fontSize: 14
    readonly property bool modified: editor.text !== savedText
    readonly property string fileName: filePath.length ? Sys.fileName(filePath) : qsTr("Untitled")

    width: 900
    height: 640
    minimumWidth: 480
    minimumHeight: 320
    visible: true
    title: (modified ? "• " : "") + fileName

    property string message: ""
    property bool messageError: false
    function notify(m, err) { message = m; messageError = err === true; msgTimer.restart() }
    Timer { id: msgTimer; interval: 5000; onTriggered: win.message = "" }

    // ---- file actions ------------------------------------------------------
    property var pendingAction: null   // run after "save changes?" is resolved

    function loadFile(path) {
        const r = DocumentIO.load(path)
        if (!r.ok) { notify(r.error, true); return false }
        filePath = r.path
        lineEnding = r.lineEnding
        encoding = r.encoding
        readOnly = r.readOnly === true
        editor.text = r.text
        savedText = r.text
        editor.cursorPosition = 0
        editor.forceActiveFocus()
        if (r.isNew) notify(qsTr("New file — it will be created when you save."), false)
        else if (readOnly) notify(qsTr("You don't have permission to change this file; use Save as to keep a copy."), true)
        return true
    }
    function saveTo(path) {
        const err = DocumentIO.save(path, editor.text, lineEnding)
        if (err.length) { notify(err, true); return false }
        filePath = DocumentIO.absolutePath(path)
        savedText = editor.text
        readOnly = false
        notify(qsTr("Saved %1").arg(Sys.fileName(path)), false)
        return true
    }
    function save() {
        if (!filePath.length || readOnly) { saveAs(); return false }
        return saveTo(filePath)
    }
    function saveAs() {
        chooser.mode = "save"
        chooser.openAt(filePath.length ? Sys.parentDir(filePath) : Sys.standardDir("documents"),
                       filePath.length ? Sys.fileName(filePath) : qsTr("Untitled.txt"))
    }
    function guard(action) {
        if (!modified) { action(); return }
        pendingAction = action
        unsavedDialog.open()
    }
    function newDocument() {
        guard(() => { filePath = ""; editor.text = ""; savedText = ""; lineEnding = "LF"; encoding = "UTF-8"; readOnly = false })
    }
    function openDialog() {
        guard(() => { chooser.mode = "open"; chooser.openAt(filePath.length ? Sys.parentDir(filePath) : Sys.standardDir("documents"), "") })
    }

    onClosing: (close) => {
        if (modified) {
            close.accepted = false
            pendingAction = () => { savedText = editor.text; Qt.quit() }
            unsavedDialog.open()
        }
    }

    Component.onCompleted: if (startFile.length) loadFile(startFile); else editor.forceActiveFocus()

    // ---- find / replace --------------------------------------------------
    function findNext(backwards) {
        const needle = findField.text
        if (!needle.length) return
        const cs = caseBox.checked
        const hay = cs ? editor.text : editor.text.toLowerCase()
        const n = cs ? needle : needle.toLowerCase()
        let i
        if (backwards) {
            i = hay.lastIndexOf(n, Math.max(0, editor.selectionStart - 1))
            if (i < 0 || i >= editor.selectionStart) i = hay.lastIndexOf(n)
        } else {
            i = hay.indexOf(n, editor.selectionEnd)
            if (i < 0) i = hay.indexOf(n)
        }
        if (i < 0) { findField.notFound = true; return }
        findField.notFound = false
        editor.select(i, i + needle.length)
    }
    function replaceOne() {
        if (editor.selectedText.length && (caseBox.checked ? editor.selectedText === findField.text
                                                         : editor.selectedText.toLowerCase() === findField.text.toLowerCase()))
            editor.remove(editor.selectionStart, editor.selectionEnd), editor.insert(editor.cursorPosition, replaceField.text)
        findNext(false)
    }
    function replaceAll() {
        const needle = findField.text
        if (!needle.length) return
        const flags = caseBox.checked ? "g" : "gi"
        const re = new RegExp(needle.replace(/[.*+?^${}()|[\]\\]/g, "\\$&"), flags)
        const matches = (editor.text.match(re) || []).length
        if (!matches) { findField.notFound = true; return }
        editor.text = editor.text.replace(re, replaceField.text.replace(/\$/g, "$$$$"))
        notify(matches === 1 ? qsTr("Replaced 1 occurrence") : qsTr("Replaced %1 occurrences").arg(matches), false)
    }
    function goToLine(n) {
        const lines = editor.text.split("\n")
        n = Math.max(1, Math.min(lines.length, n))
        let pos = 0
        for (let i = 0; i < n - 1; ++i) pos += lines[i].length + 1
        editor.cursorPosition = pos
        editor.forceActiveFocus()
    }

    // ---- shortcuts ---------------------------------------------------------
    Shortcut { sequences: [StandardKey.New]; onActivated: win.newDocument() }
    Shortcut { sequences: [StandardKey.Open]; onActivated: win.openDialog() }
    Shortcut { sequences: [StandardKey.Save]; onActivated: win.save() }
    Shortcut { sequences: ["Ctrl+Shift+S"]; onActivated: win.saveAs() }
    Shortcut { sequences: [StandardKey.Find]; onActivated: { findBar.visible = true; findField.forceActiveFocus(); findField.selectAll() } }
    Shortcut { sequences: [StandardKey.Replace, "Ctrl+H"]; onActivated: { findBar.visible = true; findBar.replacing = true; findField.forceActiveFocus() } }
    Shortcut { sequences: [StandardKey.FindNext, "F3"]; onActivated: win.findNext(false) }
    Shortcut { sequences: [StandardKey.FindPrevious, "Shift+F3"]; onActivated: win.findNext(true) }
    Shortcut { sequence: "Ctrl+G"; onActivated: { gotoDialog.open() } }
    Shortcut { sequences: [StandardKey.ZoomIn, "Ctrl+="]; onActivated: win.fontSize = Math.min(48, win.fontSize + 1) }
    Shortcut { sequences: [StandardKey.ZoomOut]; onActivated: win.fontSize = Math.max(8, win.fontSize - 1) }
    Shortcut { sequence: "Ctrl+0"; onActivated: win.fontSize = 14 }
    Shortcut { sequence: "Alt+Z"; onActivated: win.wrap = !win.wrap }
    Shortcut { sequences: [StandardKey.Close, StandardKey.Quit]; onActivated: win.close() }
    Shortcut { sequence: "Escape"; enabled: findBar.visible; onActivated: { findBar.visible = false; editor.forceActiveFocus() } }

    // ---- toolbar ------------------------------------------------------------
    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: 2
            ToolButton { icon.name: "file-plus"; onClicked: win.newDocument(); ToolTip.visible: hovered; ToolTip.text: qsTr("New (Ctrl+N)") }
            ToolButton { icon.name: "open"; onClicked: win.openDialog(); ToolTip.visible: hovered; ToolTip.text: qsTr("Open (Ctrl+O)") }
            ToolButton { icon.name: "save"; onClicked: win.save(); ToolTip.visible: hovered; ToolTip.text: qsTr("Save (Ctrl+S)") }
            ToolButton { text: qsTr("Save as…"); onClicked: win.saveAs() }
            ToolSeparator {}
            ToolButton { icon.name: "undo"; enabled: editor.canUndo; onClicked: editor.undo(); ToolTip.visible: hovered; ToolTip.text: qsTr("Undo (Ctrl+Z)") }
            ToolButton { icon.name: "redo"; enabled: editor.canRedo; onClicked: editor.redo(); ToolTip.visible: hovered; ToolTip.text: qsTr("Redo (Ctrl+Shift+Z)") }
            ToolSeparator {}
            ToolButton { icon.name: "search"; checkable: true; checked: findBar.visible
                         onClicked: { findBar.visible = !findBar.visible; if (findBar.visible) findField.forceActiveFocus() }
                         ToolTip.visible: hovered; ToolTip.text: qsTr("Find and replace (Ctrl+F)") }
            ToolButton { icon.name: "wrap"; checkable: true; checked: win.wrap; onClicked: win.wrap = !win.wrap
                         ToolTip.visible: hovered; ToolTip.text: qsTr("Word wrap (Alt+Z)") }
            ToolButton { icon.name: "code"; checkable: true; checked: win.mono; onClicked: win.mono = !win.mono
                         ToolTip.visible: hovered; ToolTip.text: qsTr("Monospaced font") }
            Item { Layout.fillWidth: true }
            Label {
                text: win.filePath.length ? win.filePath.replace(Sys.homePath, "~") : ""
                color: Theme.textSecondary
                elide: Text.ElideMiddle
                Layout.maximumWidth: 360
                Layout.rightMargin: 8
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Messages
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: win.message.length ? 36 : 0
            visible: Layout.preferredHeight > 0
            color: win.messageError ? Theme.dangerBackground : Theme.accentSoft
            Behavior on Layout.preferredHeight { NumberAnimation { duration: Theme.duration } }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 6
                Icon { name: win.messageError ? "warning" : "info"; size: 15; color: win.messageError ? Theme.danger : Theme.accent }
                Label { Layout.fillWidth: true; text: win.message; elide: Text.ElideRight }
                ToolButton { icon.name: "close"; onClicked: win.message = "" }
            }
        }

        // Find / replace
        Rectangle {
            id: findBar
            property bool replacing: false
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? findCol.implicitHeight + 16 : 0
            visible: false
            color: Theme.surfaceAlt
            ColumnLayout {
                id: findCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 6
                RowLayout {
                    TextField {
                        id: findField
                        property bool notFound: false
                        Layout.preferredWidth: 280
                        placeholderText: qsTr("Find")
                        color: notFound ? Theme.danger : Theme.text
                        onTextChanged: notFound = false
                        onAccepted: win.findNext(false)
                    }
                    ToolButton { icon.name: "chevron-up"; onClicked: win.findNext(true) }
                    ToolButton { icon.name: "chevron-down"; onClicked: win.findNext(false) }
                    CheckBox { id: caseBox; text: qsTr("Match case") }
                    Item { Layout.fillWidth: true }
                    ToolButton { text: findBar.replacing ? qsTr("Hide replace") : qsTr("Replace…"); onClicked: findBar.replacing = !findBar.replacing }
                    ToolButton { icon.name: "close"; onClicked: { findBar.visible = false; editor.forceActiveFocus() } }
                }
                RowLayout {
                    visible: findBar.replacing
                    TextField { id: replaceField; Layout.preferredWidth: 280; placeholderText: qsTr("Replace with"); onAccepted: win.replaceOne() }
                    Button { text: qsTr("Replace"); enabled: !win.readOnly; onClicked: win.replaceOne() }
                    Button { text: qsTr("Replace all"); enabled: !win.readOnly; onClicked: win.replaceAll() }
                }
            }
        }

        // Editor with line numbers
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.surface

            Flickable {
                id: flick
                anchors.fill: parent
                contentWidth: win.wrap ? width : Math.max(width, editor.implicitWidth + gutter.width)
                contentHeight: Math.max(height, editor.implicitHeight)
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {}
                ScrollBar.horizontal: ScrollBar {}

                function ensureVisible(r) {
                    if (contentX >= r.x + gutter.width - 20) contentX = Math.max(0, r.x + gutter.width - 40)
                    else if (contentX + width <= r.x + gutter.width + r.width + 20) contentX = r.x + gutter.width + r.width + 40 - width
                    if (contentY >= r.y) contentY = r.y
                    else if (contentY + height <= r.y + r.height) contentY = r.y + r.height - height + 8
                }

                // Gutter (shown when lines are not wrapped, so numbers line up)
                Rectangle {
                    id: gutter
                    visible: !win.wrap
                    width: visible ? numbers.implicitWidth + 24 : 0
                    height: Math.max(flick.height, editor.implicitHeight)
                    color: Theme.surfaceAlt
                    Text {
                        id: numbers
                        x: 12
                        y: editor.topPadding
                        horizontalAlignment: Text.AlignRight
                        font: editor.font
                        color: Theme.textTertiary
                        text: {
                            if (!gutter.visible) return ""
                            const n = editor.lineCount
                            let s = ""
                            for (let i = 1; i <= n; ++i) s += i + (i < n ? "\n" : "")
                            return s
                        }
                    }
                }

                TextArea {
                    id: editor
                    x: gutter.width
                    width: win.wrap ? flick.width - gutter.width : Math.max(implicitWidth, flick.width - gutter.width)
                    height: Math.max(implicitHeight, flick.height)
                    framed: false
                    readOnly: win.readOnly
                    wrapMode: win.wrap ? TextEdit.Wrap : TextEdit.NoWrap
                    textFormat: TextEdit.PlainText
                    selectByMouse: true
                    persistentSelection: true
                    font.family: win.mono ? Theme.monoFamily : Theme.fontFamily
                    font.pixelSize: win.fontSize
                    tabStopDistance: fontMetrics.averageCharacterWidth * 4
                    padding: 14
                    background: null
                    onCursorRectangleChanged: flick.ensureVisible(cursorRectangle)
                    FontMetrics { id: fontMetrics; font: editor.font }
                }
            }
        }

        // Status bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: Theme.window
            Rectangle { width: parent.width; height: 1; color: Theme.divider }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 20
                readonly property var pos: {
                    const before = editor.text.substring(0, editor.cursorPosition)
                    const line = before.split("\n").length
                    const col = editor.cursorPosition - before.lastIndexOf("\n")
                    return { line: line, col: col }
                }
                Label { text: qsTr("Ln %1, Col %2").arg(parent.pos.line).arg(parent.pos.col); font.pixelSize: Theme.fontCaption; color: Theme.textSecondary }
                Label { text: editor.length === 1 ? qsTr("1 character") : qsTr("%1 characters").arg(editor.length); font.pixelSize: Theme.fontCaption; color: Theme.textSecondary }
                Item { Layout.fillWidth: true }
                Label { text: win.readOnly ? qsTr("Read-only") : ""; visible: win.readOnly; font.pixelSize: Theme.fontCaption; color: Theme.warning }
                Label { text: win.fontSize !== 14 ? Math.round(win.fontSize / 14 * 100) + "%" : ""; font.pixelSize: Theme.fontCaption; color: Theme.textSecondary }
                Label { text: win.lineEnding === "CRLF" ? "Windows (CRLF)" : "Unix (LF)"; font.pixelSize: Theme.fontCaption; color: Theme.textSecondary }
                Label { text: win.encoding; font.pixelSize: Theme.fontCaption; color: Theme.textSecondary }
            }
        }
    }

    FileChooser {
        id: chooser
        onAccepted: (path) => {
            if (mode === "save") win.saveTo(path)
            else win.loadFile(path)
        }
    }

    Dialog {
        id: unsavedDialog
        title: qsTr("Save changes to “%1”?").arg(win.fileName)
        width: 440
        contentItem: Label {
            text: qsTr("Your changes will be lost if you don't save them.")
            wrapMode: Text.Wrap
            color: Theme.textSecondary
        }
        footer: DialogButtonBox {
            Button { text: qsTr("Cancel"); DialogButtonBox.buttonRole: DialogButtonBox.RejectRole }
            Button {
                text: qsTr("Don't save")
                DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole
                onClicked: { unsavedDialog.close(); const a = win.pendingAction; win.pendingAction = null; if (a) a() }
            }
            Button {
                text: qsTr("Save")
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
        }
        onAccepted: {
            const a = win.pendingAction
            win.pendingAction = null
            if (win.filePath.length && !win.readOnly) {
                if (win.saveTo(win.filePath) && a) a()
            } else {
                win.saveAs()   // the user can run the action again after saving
            }
        }
        onRejected: win.pendingAction = null
    }

    Dialog {
        id: gotoDialog
        title: qsTr("Go to line")
        width: 320
        onOpened: { lineField.text = ""; lineField.forceActiveFocus() }
        contentItem: TextField {
            id: lineField
            placeholderText: qsTr("Line number (1–%1)").arg(editor.text.split("\n").length)
            inputMethodHints: Qt.ImhDigitsOnly
            validator: IntValidator { bottom: 1 }
            onAccepted: gotoDialog.accept()
        }
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: if (lineField.text.length) win.goToLine(Number(lineField.text))
    }
}
