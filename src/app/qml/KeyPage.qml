// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import EasyMorse

// Morse key; what is keyed is decoded at the bottom of the window
Page {
    id: root

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

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
                  + " · " + qsTr("Decoded at %1 WPM").arg(Math.round(Trainer.decoderSpeed))
                  + (MorseSettings.decoderAutoAdapt ? "" : " " + qsTr("(locked)"))
            opacity: 0.7
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }
    }
}
