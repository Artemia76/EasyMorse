// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EasyMorse

// Keypad of the copy practice, for touch screens: one key per character
// learnt (the newest one highlighted) and a key to erase. The keys never
// take the focus, so the physical keyboard keeps working.
Item {
    id: root

    property string characters
    signal typed(string character)
    signal erased()

    readonly property int keyCount: characters.length + 1
    // Letters A to Z, digits, then signs: easier to find than the Koch order
    readonly property var sorted: {
        const rank = (c) => c >= "A" && c <= "Z" ? 0 : c >= "0" && c <= "9" ? 1 : 2
        return characters.split("").sort((a, b) => rank(a) - rank(b) || (a < b ? -1 : a > b ? 1 : 0))
    }
    readonly property int spacing: 6
    // As many columns as fit, keys at least 40 px wide
    readonly property int columns: Math.max(4, Math.min(keyCount, Math.floor((width + spacing) / (40 + spacing))))
    readonly property int rows: Math.ceil(keyCount / columns)
    readonly property real keyWidth: Math.min(96, (width - (columns - 1) * spacing) / columns)
    readonly property real keyHeight: Math.max(32, Math.min(64, (height - (rows - 1) * spacing) / rows))

    implicitHeight: rows * 52 + (rows - 1) * spacing

    component PadKey: AbstractButton {
        id: key
        property bool highlighted
        focusPolicy: Qt.NoFocus
        width: root.keyWidth
        height: root.keyHeight
        Accessible.name: text
        background: Rectangle {
            radius: 10
            color: key.down ? Theme.accent : key.highlighted ? Theme.okBackground : Theme.raised
            border.width: 1
            border.color: key.highlighted ? Theme.ok : Theme.line
            opacity: key.enabled ? 1 : 0.5
        }
        contentItem: Label {
            text: key.text
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.family: Theme.monoFont
            font.pixelSize: Math.min(26, key.height * 0.45)
            font.weight: Font.DemiBold
            color: key.down ? Theme.accentInk : key.highlighted ? Theme.okText : Theme.text
        }
    }

    Grid {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        columns: root.columns
        spacing: root.spacing

        Repeater {
            model: root.sorted
            PadKey {
                required property string modelData
                text: modelData
                // The newest character of the course
                highlighted: modelData === root.characters.charAt(root.characters.length - 1)
                onPressed: root.typed(modelData)
            }
        }
        PadKey {
            text: "⌫"
            Accessible.name: qsTr("Erase")
            onPressed: root.erased()
            autoRepeat: true
        }
    }
}
