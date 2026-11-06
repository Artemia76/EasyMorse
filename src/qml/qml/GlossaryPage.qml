// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import EasyMorse

// Morse table: click a character to hear it
Page {
    id: root

    signal playRequested(string text)

    GridView {
        id: grid
        anchors.fill: parent
        anchors.margins: 8
        clip: true
        cellWidth: Math.max(110, width / Math.max(1, Math.floor(width / 140)))
        cellHeight: 76
        model: GlossaryModel {}
        ScrollBar.vertical: ScrollBar {}

        delegate: ItemDelegate {
            id: cell
            required property string character
            required property string pattern

            width: grid.cellWidth - 6
            height: grid.cellHeight - 6
            enabled: !Trainer.playing && Trainer.audioRunning
            onClicked: root.playRequested(character)

            contentItem: RowLayout {
                spacing: 12
                Label {
                    text: cell.character
                    font.pixelSize: 26
                    font.bold: true
                    Layout.preferredWidth: 32
                    horizontalAlignment: Text.AlignHCenter
                }
                Label {
                    text: cell.pattern.replace(/\./g, "·").replace(/-/g, "−")
                    font.pixelSize: 26
                    color: Material.accent
                    Layout.fillWidth: true
                }
            }
        }
    }
}
