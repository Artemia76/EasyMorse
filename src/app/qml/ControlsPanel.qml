// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EasyMorse

// Speed, tone and band conditions
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
            }
        }

        // What the band sounds like: noises, fading, stations around
        Card {
            title: qsTr("Band conditions")
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 16

            ColumnLayout {
                anchors.fill: parent
                spacing: 6

                ComboBox {
                    id: preset
                    // In the order of Settings::BandPreset
                    model: [qsTr("Clean"), qsTr("Quiet band"), qsTr("40 m in summer"), qsTr("Contest"),
                            qsTr("Aurora"), qsTr("Urban"), qsTr("Custom")]
                    currentIndex: MorseSettings.bandPreset
                    onActivated: (index) => MorseSettings.bandPreset = index
                    Layout.fillWidth: true
                    Accessible.name: qsTr("Band conditions")
                }
                Label {
                    text: [qsTr("The signal alone, as from a practice oscillator."),
                           qsTr("A light hiss and slow, shallow fading."),
                           qsTr("Static crashes of thunderstorms over the hiss."),
                           qsTr("Busy band: stations call CQ and exchange reports above and below you."),
                           qsTr("Auroral propagation: a rasping, fluttering tone that fades fast."),
                           qsTr("Mains buzz and a stray carrier from the neighbourhood."),
                           qsTr("Your own mix of the settings below.")][preset.currentIndex] ?? ""
                    wrapMode: Text.WordWrap
                    color: Theme.muted
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }
                Button {
                    id: advanced
                    text: checked ? qsTr("Hide the details") : qsTr("Adjust the details")
                    checkable: true
                    flat: true
                    Layout.alignment: Qt.AlignLeft
                }

                ColumnLayout {
                    visible: advanced.checked
                    spacing: 6
                    Layout.fillWidth: true

                    LabeledSlider {
                        text: qsTr("Background noise")
                        format: qsTr("%1 %")
                        from: 0; to: 1; stepSize: 0.01
                        display: (v) => Math.round(v * 100)
                        value: MorseSettings.noiseLevel
                        onMoved: (v) => MorseSettings.noiseLevel = v
                    }
                    // Receiver filter: a narrow one removes noise and neighbours
                    LabeledSlider {
                        text: qsTr("Bandwidth")
                        format: qsTr("%1 Hz")
                        from: 100; to: 3000; stepSize: 50
                        value: MorseSettings.bandwidth
                        onMoved: (v) => MorseSettings.bandwidth = v
                    }
                    // Fading of the received signals (not of the own key)
                    LabeledSlider {
                        text: qsTr("QSB (fading)")
                        format: qsTr("%1 %")
                        from: 0; to: 100; stepSize: 5
                        value: MorseSettings.qsbDepth
                        onMoved: (v) => MorseSettings.qsbDepth = v
                    }
                    LabeledSlider {
                        text: qsTr("Fading period")
                        format: qsTr("about %1 s")
                        // Setting in 1/100 Hz: shown as the time between two fades
                        from: 5; to: 100; stepSize: 1
                        display: (v) => Math.round(100 / v)
                        value: MorseSettings.qsbRate
                        enabled: MorseSettings.qsbDepth > 0
                        onMoved: (v) => MorseSettings.qsbRate = v
                    }
                    LabeledSlider {
                        text: qsTr("Auroral flutter")
                        format: qsTr("%1 Hz")
                        from: 0; to: 50; stepSize: 1
                        value: MorseSettings.flutter
                        onMoved: (v) => MorseSettings.flutter = v
                    }
                    LabeledSlider {
                        text: qsTr("Static crashes (QRN)")
                        format: qsTr("%1 %")
                        from: 0; to: 100; stepSize: 5
                        value: MorseSettings.qrnLevel
                        onMoved: (v) => MorseSettings.qrnLevel = v
                    }
                    LabeledSlider {
                        text: qsTr("Crashes per minute")
                        from: 0; to: 60; stepSize: 1
                        value: MorseSettings.qrnRate
                        enabled: MorseSettings.qrnLevel > 0
                        onMoved: (v) => MorseSettings.qrnRate = v
                    }
                    LabeledSlider {
                        text: qsTr("Man made noise")
                        format: qsTr("%1 %")
                        from: 0; to: 100; stepSize: 5
                        value: MorseSettings.manMade
                        onMoved: (v) => MorseSettings.manMade = v
                    }
                    LabeledSlider {
                        text: qsTr("Stations around (contest)")
                        from: 0; to: 8; stepSize: 1
                        value: MorseSettings.contestStations
                        onMoved: (v) => MorseSettings.contestStations = v
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("Automatic gain control (AGC)")
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                        Switch {
                            checked: MorseSettings.agc
                            onToggled: MorseSettings.agc = checked
                            Accessible.name: qsTr("Automatic gain control (AGC)")
                        }
                    }
                }
            }
        }
    }
}
