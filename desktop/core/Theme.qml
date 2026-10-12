pragma Singleton
import QtQuick
import Meridian.Core

// Meridian design tokens. Everything visual reads from here, so light/dark
// and accent changes made in Settings apply live across the desktop.
QtObject {
    id: theme

    readonly property bool dark: Settings.darkMode
    readonly property bool animations: Settings.animations
    readonly property bool transparency: Settings.transparency

    // ---- colour ------------------------------------------------------------
    readonly property color accentBase: Settings.accentColor
    // Dark mode uses a lighter tint of the accent for contrast on dark surfaces.
    readonly property color accent: dark ? Qt.lighter(accentBase, 1.55) : accentBase
    readonly property color accentHover: dark ? Qt.lighter(accent, 1.08) : Qt.darker(accent, 1.08)
    readonly property color accentPressed: dark ? Qt.darker(accent, 1.08) : Qt.darker(accent, 1.18)
    readonly property color onAccent: dark ? "#0B1614" : "#FFFFFF"
    readonly property color accentSoft: Qt.rgba(accent.r, accent.g, accent.b, dark ? 0.22 : 0.14)

    readonly property color window: dark ? "#1C2020" : "#F5F6F5"
    readonly property color surface: dark ? "#252A2A" : "#FFFFFF"
    readonly property color surfaceAlt: dark ? "#2C3232" : "#EEF0EF"
    readonly property color sidebar: dark ? "#202525" : "#EDEFEE"
    // Taskbar: lightly translucent over the wallpaper.
    readonly property color panel: transparency
                                    ? (dark ? Qt.rgba(0.125, 0.14, 0.14, 0.92) : Qt.rgba(0.965, 0.972, 0.968, 0.92))
                                    : (dark ? "#202424" : "#F6F7F6")
    // Flyouts sit over windows; without background blur, high translucency
    // looks muddy, so they are nearly opaque.
    readonly property color flyout: transparency
                                    ? (dark ? Qt.rgba(0.137, 0.157, 0.157, 0.97) : Qt.rgba(0.973, 0.976, 0.973, 0.97))
                                    : (dark ? "#232828" : "#F8F9F8")
    readonly property color panelSolid: dark ? "#232828" : "#F8F9F8"
    readonly property color border: dark ? Qt.rgba(1, 1, 1, 0.08) : Qt.rgba(0, 0, 0, 0.09)
    readonly property color borderStrong: dark ? Qt.rgba(1, 1, 1, 0.16) : Qt.rgba(0, 0, 0, 0.18)
    readonly property color divider: dark ? Qt.rgba(1, 1, 1, 0.07) : Qt.rgba(0, 0, 0, 0.07)

    readonly property color text: dark ? "#ECEFEE" : "#1A1E1D"
    readonly property color textSecondary: dark ? "#A7B0AE" : "#5A6462"
    readonly property color textTertiary: dark ? "#7E8886" : "#89928F"
    readonly property color textDisabled: dark ? "#5F6867" : "#AEB5B3"

    readonly property color hover: dark ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(0, 0, 0, 0.045)
    readonly property color pressed: dark ? Qt.rgba(1, 1, 1, 0.10) : Qt.rgba(0, 0, 0, 0.08)
    readonly property color control: dark ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(1, 1, 1, 0.85)
    readonly property color controlBorder: dark ? Qt.rgba(1, 1, 1, 0.10) : Qt.rgba(0, 0, 0, 0.13)
    readonly property color inputBackground: dark ? Qt.rgba(1, 1, 1, 0.05) : "#FFFFFF"
    readonly property color selection: Qt.rgba(accent.r, accent.g, accent.b, dark ? 0.32 : 0.22)

    readonly property color danger: dark ? "#F28B86" : "#C2372F"
    readonly property color dangerBackground: dark ? Qt.rgba(0.95, 0.35, 0.33, 0.16) : Qt.rgba(0.76, 0.22, 0.18, 0.10)
    readonly property color success: dark ? "#74CF97" : "#2D8650"
    readonly property color warning: dark ? "#E8BB5E" : "#B07A12"
    readonly property color scrim: Qt.rgba(0, 0, 0, dark ? 0.5 : 0.32)

    // ---- typography ------------------------------------------------------
    readonly property string fontFamily: "Inter"
    readonly property string monoFamily: "JetBrains Mono"
    readonly property int fontCaption: 12
    readonly property int fontBody: 14
    readonly property int fontBodyLarge: 15
    readonly property int fontSubtitle: 17
    readonly property int fontTitle: 22
    readonly property int fontDisplay: 34

    // ---- shape & spacing -------------------------------------------------
    readonly property int radiusSmall: 4
    readonly property int radius: 6
    readonly property int radiusLarge: 10
    readonly property int radiusXL: 14
    readonly property int spacingXS: 4
    readonly property int spacingS: 8
    readonly property int spacingM: 12
    readonly property int spacingL: 16
    readonly property int spacingXL: 24
    readonly property int controlHeight: 34

    // ---- motion ----------------------------------------------------------
    readonly property int durationFast: animations ? 110 : 0
    readonly property int duration: animations ? 190 : 0
    readonly property int durationSlow: animations ? 280 : 0
    readonly property int easing: Easing.OutCubic

    // Accent presets offered in Settings > Personalisation.
    readonly property var accentPresets: [
        { name: "Pine",     color: "#1F8A7A" },
        { name: "Harbor",   color: "#2F6EA8" },
        { name: "Heather",  color: "#7D57A8" },
        { name: "Clay",     color: "#B4552F" },
        { name: "Ochre",    color: "#A57712" },
        { name: "Rosewood", color: "#B03E62" },
        { name: "Moss",     color: "#5C7D2E" },
        { name: "Graphite", color: "#55606A" }
    ]

    function alpha(c, a) { return Qt.rgba(c.r, c.g, c.b, a) }
    // Hex string without '#', as the icon provider expects.
    function hex(c) { return String(c).replace("#", "") }
}
