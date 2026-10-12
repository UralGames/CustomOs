pragma Singleton
import QtQuick

// State shared by the shell's surfaces (taskbar, overlay panels, tooltip).
QtObject {
    // Open panel in the overlay surface:
    // "", "launcher", "quick", "calendar", "taskview", "menu"
    property string panel: ""
    readonly property int taskbarHeight: 48

    // Context menu requested by the taskbar (rendered in the overlay).
    property var menuItems: []      // [{ text, icon, enabled, trigger }]
    property real menuX: 0

    // Tooltip above the taskbar (its own small surface).
    property string tooltipText: ""
    property real tooltipX: 0

    signal osd(string icon, string text)

    function toggle(name) {
        hideTooltip()
        panel = panel === name ? "" : name
    }
    function open(name) {
        hideTooltip()
        panel = name
    }
    function closePanel() {
        panel = ""
    }
    function openMenu(x, items) {
        hideTooltip()
        menuItems = items
        menuX = x
        panel = "menu"
    }
    function showTooltip(text, x) {
        if (panel !== "") return
        tooltipText = text
        tooltipX = x
    }
    function hideTooltip() {
        tooltipText = ""
    }
}
