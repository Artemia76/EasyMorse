// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import EasyMorse

// Morse key and real time decoding of what is keyed
Page {
    id: root

    Connections {
        target: Trainer
        function onCharDecoded(pText, pPattern) {
            // Unknown patterns are shown as received to help the learner
            decoded.insert(decoded.length, pText.length > 0 ? pText : "[" + pPattern + "]")
        }
        function onWordGap() { decoded.insert(decoded.length, " ") }
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 16
        columns: root.width > root.height ? 2 : 1
        columnSpacing: 24
        rowSpacing: 16

        ColumnLayout {
            spacing: 12
            Layout.fillWidth: true
            Layout.fillHeight: true

            KeyPad {
                id: pad
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 160
            }
            Label {
                // Dots and dashes of the character being keyed
                text: Trainer.decoderPattern.replace(/\./g, "·").replace(/-/g, "−") || " "
                font.pixelSize: 36
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("Space bar (key focused), Alt key or serial key")
                opacity: 0.7
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
            }
        }

        ColumnLayout {
            spacing: 8
            Layout.fillWidth: true
            Layout.fillHeight: true

            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: qsTr("Decoded at %1 WPM").arg(Math.round(Trainer.decoderSpeed))
                          + (MorseSettings.decoderAutoAdapt ? "" : " " + qsTr("(locked)"))
                    font.bold: true
                    Layout.fillWidth: true
                }
                Button {
                    text: qsTr("Clear")
                    flat: true
                    onClicked: decoded.clear()
                }
            }
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                TextArea {
                    id: decoded
                    readOnly: true
                    wrapMode: TextEdit.Wrap
                    font.pixelSize: 24
                    font.family: "monospace"
                    // Keep the last characters visible
                    onTextChanged: cursorPosition = length

                    // Hint instead of placeholderText, which Material turns into a floating label
                    Label {
                        anchors.fill: parent
                        anchors.margins: 16
                        visible: decoded.length === 0
                        text: qsTr("Decoded text appears here as you key")
                        wrapMode: Text.WordWrap
                        opacity: 0.5
                    }
                }
            }
        }
    }
}
