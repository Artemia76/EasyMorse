// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EasyMorse

// Tone, noise and speed settings
ScrollView {
    id: root

    contentWidth: availableWidth
    clip: true

    ColumnLayout {
        width: root.availableWidth
        spacing: 8

        Item { Layout.preferredHeight: 4 }

        Label {
            text: qsTr("Sound")
            font.bold: true
            Layout.leftMargin: 16
        }
        LabeledSlider {
            text: qsTr("Tone: %1 Hz")
            from: 200; to: 1200; stepSize: 10
            value: MorseSettings.toneFrequency
            enabled: !Trainer.playing
            onMoved: (v) => MorseSettings.toneFrequency = v
            Layout.leftMargin: 16; Layout.rightMargin: 16
        }
        LabeledSlider {
            text: qsTr("Volume: %1 %")
            from: 0; to: 100; stepSize: 1
            value: MorseSettings.volume
            onMoved: (v) => MorseSettings.volume = v
            Layout.leftMargin: 16; Layout.rightMargin: 16
        }
        LabeledSlider {
            text: qsTr("Background noise: %1 %")
            from: 0; to: 1; stepSize: 0.01
            display: (v) => Math.round(v * 100)
            value: MorseSettings.noiseLevel
            enabled: !Trainer.playing
            onMoved: (v) => MorseSettings.noiseLevel = v
            Layout.leftMargin: 16; Layout.rightMargin: 16
        }
        LabeledSlider {
            text: qsTr("Noise filter: %1")
            from: 0; to: 1000; stepSize: 10
            value: MorseSettings.noiseFilter
            enabled: !Trainer.playing
            onMoved: (v) => MorseSettings.noiseFilter = v
            Layout.leftMargin: 16; Layout.rightMargin: 16
        }

        Label {
            text: qsTr("Speed")
            font.bold: true
            Layout.leftMargin: 16
            Layout.topMargin: 8
        }
        LabeledSlider {
            text: qsTr("Characters: %1 WPM")
            from: 5; to: 40; stepSize: 1
            value: MorseSettings.charSpeed
            enabled: !Trainer.playing
            onMoved: (v) => MorseSettings.charSpeed = v
            Layout.leftMargin: 16; Layout.rightMargin: 16
        }
        LabeledSlider {
            text: qsTr("Overall: %1 WPM")
            from: 5; to: 40; stepSize: 1
            value: MorseSettings.wordSpeed
            enabled: !Trainer.playing
            onMoved: (v) => MorseSettings.wordSpeed = v
            Layout.leftMargin: 16; Layout.rightMargin: 16
        }
        Switch {
            text: qsTr("Farnsworth spacing")
            checked: MorseSettings.farnsworth
            enabled: !Trainer.playing
            onToggled: MorseSettings.farnsworth = checked
            Layout.leftMargin: 8
        }
        Label {
            text: qsTr("Characters are sent at their own speed, spaces are stretched to reach the overall speed.")
            wrapMode: Text.WordWrap
            opacity: 0.7
            font.pixelSize: 12
            Layout.fillWidth: true
            Layout.leftMargin: 16; Layout.rightMargin: 16
        }
    }
}
