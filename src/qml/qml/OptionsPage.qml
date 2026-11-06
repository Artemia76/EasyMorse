// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EasyMorse

// Devices, decoder and interface options; changes apply at once
Page {
    id: root

    ScrollView {
        id: scroll
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: scroll.availableWidth
            spacing: 12

            GroupBox {
                title: qsTr("Sound")
                Layout.fillWidth: true
                Layout.margins: 12

                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    columnSpacing: 12

                    Label { text: qsTr("Output sound card") }
                    ComboBox {
                        id: deviceBox
                        Layout.fillWidth: true
                        // Empty name: system default output
                        model: [""].concat(Trainer.audioDevices())
                        displayText: currentIndex <= 0 ? qsTr("System default") : currentText
                        delegate: MenuItem {
                            required property string modelData
                            required property int index
                            width: deviceBox.width
                            text: index === 0 ? qsTr("System default") : modelData
                            highlighted: deviceBox.highlightedIndex === index
                        }
                        Component.onCompleted: currentIndex = Math.max(0, model.indexOf(MorseSettings.audioDevice))
                        onActivated: (index) => MorseSettings.audioDevice = model[index]
                    }

                    Label { text: qsTr("Buffer size (keyer latency)") }
                    SpinBox {
                        from: 2
                        to: 100
                        value: MorseSettings.audioPeriodMs
                        textFromValue: (value) => qsTr("%1 ms").arg(value)
                        valueFromText: (text) => parseInt(text)
                        // Each change restarts the sound card: wait for the last click
                        onValueModified: periodTimer.restart()
                        Timer {
                            id: periodTimer
                            interval: 600
                            onTriggered: MorseSettings.audioPeriodMs = parent.value
                        }
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Smaller is more responsive. Increase it if you hear crackles.")
                    }
                }
            }

            GroupBox {
                title: qsTr("Serial key")
                visible: Trainer.serialPorts().length > 0 || MorseSettings.serialEnabled
                Layout.fillWidth: true
                Layout.leftMargin: 12
                Layout.rightMargin: 12

                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    columnSpacing: 12

                    Label { text: qsTr("Enabled") }
                    Switch {
                        checked: MorseSettings.serialEnabled
                        onToggled: MorseSettings.serialEnabled = checked
                    }

                    Label { text: qsTr("Port") }
                    RowLayout {
                        Layout.fillWidth: true
                        ComboBox {
                            id: portBox
                            editable: true
                            model: Trainer.serialPorts()
                            Layout.fillWidth: true
                            Component.onCompleted: editText = MorseSettings.serialPort
                            onActivated: MorseSettings.serialPort = currentText
                            onAccepted: MorseSettings.serialPort = editText
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Choose a port or type a device path")
                        }
                        ToolButton {
                            text: "⟳"
                            onClicked: portBox.model = Trainer.serialPorts()
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Refresh the list of ports")
                        }
                    }
                }
            }

            GroupBox {
                title: qsTr("Decoder")
                Layout.fillWidth: true
                Layout.leftMargin: 12
                Layout.rightMargin: 12

                ColumnLayout {
                    anchors.fill: parent
                    Switch {
                        text: qsTr("Adapts to the sender speed")
                        checked: MorseSettings.decoderAutoAdapt
                        onToggled: MorseSettings.decoderAutoAdapt = checked
                    }
                    Label {
                        text: qsTr("When off, the decoder expects the character speed.")
                        opacity: 0.7
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }

            GroupBox {
                title: qsTr("Interface")
                Layout.fillWidth: true
                Layout.leftMargin: 12
                Layout.rightMargin: 12
                Layout.bottomMargin: 12

                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    columnSpacing: 12

                    Label { text: qsTr("Language") }
                    ComboBox {
                        Layout.fillWidth: true
                        model: UiSettings.languages
                        textRole: "name"
                        valueRole: "code"
                        Component.onCompleted: currentIndex = indexOfValue(UiSettings.language)
                        onActivated: UiSettings.language = currentValue
                    }

                    Label { text: qsTr("Theme") }
                    ComboBox {
                        Layout.fillWidth: true
                        model: [qsTr("System"), qsTr("Light"), qsTr("Dark")]
                        currentIndex: UiSettings.theme
                        onActivated: (index) => UiSettings.theme = index
                    }
                }
            }
        }
    }
}
