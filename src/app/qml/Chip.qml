// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EasyMorse

// Small rounded status label, with an optional light
Rectangle {
    id: root

    property alias text: label.text
    property alias textFormat: label.textFormat
    property color foreground: Theme.textSoft
    // Transparent: no light
    property color light: "transparent"

    implicitWidth: row.implicitWidth + 24
    implicitHeight: 30
    radius: height / 2
    color: Theme.raised

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: 8
        Rectangle {
            visible: root.light.a > 0
            width: 8; height: 8; radius: 4
            color: root.light
        }
        Label {
            id: label
            color: root.foreground
            font.weight: Font.Medium
        }
    }
}
