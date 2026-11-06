// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import EasyMorse

// Text to Morse: type a text and listen to it
Page {
    id: root

    property string playedText
    property int playedIndex: -1

    // Played text with the character being sent highlighted
    function highlighted(text, index) {
        const escape = (s) => s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
        if (index < 0 || index >= text.length)
            return escape(text)
        return escape(text.slice(0, index))
             + "<span style=\"background-color:" + Material.accent + "; color:black\">"
             + escape(text[index]) + "</span>" + escape(text.slice(index + 1))
    }

    function playOrStop() {
        if (Trainer.playing) {
            Trainer.stop()
        } else if (message.text.length > 0) {
            root.playedText = message.text
            root.playedIndex = -1
            Trainer.play(message.text)
        }
    }

    Connections {
        target: Trainer
        function onCharPlayed(pIndex) { root.playedIndex = pIndex }
        function onPlayingChanged() { if (!Trainer.playing) root.playedIndex = -1 }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        RowLayout {
            spacing: 8
            Layout.fillWidth: true

            TextField {
                id: message
                placeholderText: qsTr("Text to send in Morse")
                text: "CQ CQ DE F4IKZ K"
                Layout.fillWidth: true
                onAccepted: root.playOrStop()
            }
            Button {
                text: Trainer.playing ? qsTr("Stop") : qsTr("Play")
                // Accent button with dark text, readable in both themes
                Material.background: Material.accent
                Material.foreground: Material.color(Material.Grey, Material.Shade900)
                enabled: Trainer.playing || (message.text.length > 0 && Trainer.audioRunning)
                onClicked: root.playOrStop()
            }
        }

        Label {
            text: root.highlighted(root.playedText, root.playedIndex)
            // Rich text: styled text has no background color
            textFormat: Text.RichText
            wrapMode: Text.WrapAnywhere
            font.pixelSize: 32
            font.family: "monospace"
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("Hold the Alt key or use the Key tab to answer.")
            opacity: 0.7
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Item { Layout.fillHeight: true }
    }
}
