// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import EasyMorse

// Morse table: click a character to hear it
Page {
    id: root

    background: null

    signal playRequested(string text)

    GridView {
        id: grid
        anchors.fill: parent
        anchors.margins: 16
        clip: true
        cellWidth: Math.max(110, width / Math.max(1, Math.floor(width / 140)))
        cellHeight: 80
        model: GlossaryModel {}
        ScrollBar.vertical: ScrollBar {}

        delegate: ItemDelegate {
            id: cell
            required property string character
            required property string pattern

            width: grid.cellWidth - 10
            height: grid.cellHeight - 10
            background: Rectangle {
                radius: 12
                color: cell.hovered ? Theme.raised : Theme.surface
            }
            enabled: !Trainer.playing && Trainer.audioRunning
            onClicked: root.playRequested(character)

            contentItem: RowLayout {
                spacing: 12
                Label {
                    text: cell.character
                    font.family: Theme.monoFont
                    font.pixelSize: 26
                    font.bold: true
                    Layout.preferredWidth: 32
                    horizontalAlignment: Text.AlignHCenter
                }
                Label {
                    text: Theme.pattern(cell.pattern)
                    font.family: Theme.monoFont
                    font.pixelSize: 24
                    color: Theme.accentText
                    Layout.fillWidth: true
                }
            }
        }
    }
}
