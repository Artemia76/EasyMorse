// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EasyMorse

// Speed, tone and noise settings
ScrollView {
    id: root

    contentWidth: availableWidth
    clip: true

    ColumnLayout {
        width: root.availableWidth
        spacing: 16

        Card {
            title: qsTr("Speed")
            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            ColumnLayout {
                anchors.fill: parent
                spacing: 6

                LabeledSlider {
                    text: qsTr("Characters")
                    format: qsTr("%1 WPM")
                    from: 5; to: 40; stepSize: 1
                    value: MorseSettings.charSpeed
                    enabled: !Trainer.playing
                    onMoved: (v) => MorseSettings.charSpeed = v
                }
                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: qsTr("Farnsworth spacing")
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Switch {
                        checked: MorseSettings.farnsworth
                        enabled: !Trainer.playing
                        onToggled: MorseSettings.farnsworth = checked
                        Accessible.name: qsTr("Farnsworth spacing")
                    }
                }
                LabeledSlider {
                    text: qsTr("Effective")
                    format: qsTr("%1 WPM")
                    // Only used with Farnsworth spacing, and never faster than the characters
                    from: 5; to: Math.max(5, MorseSettings.charSpeed); stepSize: 1
                    value: Math.min(MorseSettings.wordSpeed, MorseSettings.charSpeed)
                    enabled: !Trainer.playing && MorseSettings.farnsworth
                    onMoved: (v) => MorseSettings.wordSpeed = v
                }
                Label {
                    text: qsTr("Characters are sent at their own speed, spaces are stretched to reach the effective speed.")
                    wrapMode: Text.WordWrap
                    color: Theme.muted
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }
            }
        }

        Card {
            title: qsTr("Sound")
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 16

            ColumnLayout {
                anchors.fill: parent
                spacing: 6

                LabeledSlider {
                    text: qsTr("Tone")
                    format: qsTr("%1 Hz")
                    from: 200; to: 1200; stepSize: 10
                    value: MorseSettings.toneFrequency
                    enabled: !Trainer.playing
                    onMoved: (v) => MorseSettings.toneFrequency = v
                }
                LabeledSlider {
                    text: qsTr("Volume")
                    format: qsTr("%1 %")
                    from: 0; to: 100; stepSize: 1
                    value: MorseSettings.volume
                    onMoved: (v) => MorseSettings.volume = v
                }
                LabeledSlider {
                    text: qsTr("Background noise")
                    format: qsTr("%1 %")
                    from: 0; to: 1; stepSize: 0.01
                    display: (v) => Math.round(v * 100)
                    value: MorseSettings.noiseLevel
                    enabled: !Trainer.playing
                    onMoved: (v) => MorseSettings.noiseLevel = v
                }
                LabeledSlider {
                    text: qsTr("Noise filter")
                    from: 0; to: 1000; stepSize: 10
                    value: MorseSettings.noiseFilter
                    enabled: !Trainer.playing
                    onMoved: (v) => MorseSettings.noiseFilter = v
                }
            }
        }
    }
}
