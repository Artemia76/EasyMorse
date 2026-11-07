// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EasyMorse

// Morse key; what is keyed is decoded at the bottom of the window
Page {
    id: root

    background: null

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            // Dots and dashes of the character being keyed
            text: Theme.pattern(Trainer.decoderPattern).split("").join(" ") || " "
            font.family: Theme.monoFont
            font.pixelSize: 44
            color: Theme.accentText
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
            Layout.preferredHeight: 60
        }
        KeyPad {
            id: pad
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 160
            Layout.maximumHeight: 340
            Layout.alignment: Qt.AlignHCenter
        }
        Label {
            text: qsTr("Space bar (key focused), Alt key or serial key")
            color: Theme.muted
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }
        Item { Layout.fillHeight: true; Layout.maximumHeight: 40 }
    }
}
