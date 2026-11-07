// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

pragma Singleton

import QtQuick

// Colors and fonts of the interface: a graphite "radio set" look with one
// amber accent for what sends or is active, teal kept for "connected"
QtObject {
    // Set by the main window from the theme chosen in the options
    property bool dark: true

    readonly property string uiFont: "Inter"
    readonly property string monoFont: "JetBrains Mono"

    readonly property color ground: dark ? "#121418" : "#F4F3EF"
    // Header, navigation and side panels
    readonly property color chrome: dark ? "#16191E" : "#FBFAF7"
    readonly property color surface: dark ? "#1B1E24" : "#FFFFFF"
    readonly property color raised: dark ? "#22262E" : "#ECEAE4"
    readonly property color line: dark ? "#2A2E37" : "#DDDAD2"
    readonly property color lineStrong: dark ? "#3A3F4A" : "#C4C0B6"
    readonly property color text: dark ? "#E8E6E1" : "#1B1D22"
    readonly property color textSoft: dark ? "#C9CDD4" : "#3A3E47"
    readonly property color muted: dark ? "#A3A8B3" : "#5F6470"

    // Accent: fills (with accentInk text) and, darker on light, text
    readonly property color accent: "#F5B83D"
    readonly property color accentLight: "#FFD27A"
    readonly property color accentDeep: "#C98B16"
    readonly property color accentText: dark ? "#F5B83D" : "#8A5A00"
    readonly property color accentInk: "#121418"

    // Connected stations
    readonly property color ok: dark ? "#5CC8B8" : "#1E8C7E"
    readonly property color okText: dark ? "#7FD8C9" : "#17695F"
    readonly property color okBackground: dark ? "#1F2D2B" : "#DDF1EE"
    readonly property color error: dark ? "#F07167" : "#B3261E"

    // Reception strip at the bottom of the window
    readonly property color tape: dark ? "#0D0F12" : "#ECEAE4"

    readonly property int radius: 14

    // "·−·" from ".-."
    function pattern(pText) {
        return pText.replace(/\./g, "·").replace(/-/g, "−")
    }
}
