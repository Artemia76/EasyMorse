// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EasyMorse

// Devices, decoder and interface options; changes apply at once
Page {
    id: root

    // Key of the keyboard for a role, changed by pressing the new one
    component KeyChooser: RowLayout {
        id: chooser
        required property string role
        required property string value
        readonly property bool waiting: Keyboard.capturing === role
        spacing: 8
        Label {
            // Escape is a choice for the key that frees the mouse
            text: chooser.waiting ? (chooser.role === "release" ? qsTr("Press a key…") : qsTr("Press a key… (Escape to cancel)"))
                                  : Keyboard.keyName(chooser.value)
            font.family: chooser.waiting ? Theme.uiFont : Theme.monoFont
            color: chooser.waiting ? Theme.accentText : Theme.text
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
        Button {
            text: chooser.waiting ? qsTr("Cancel") : qsTr("Change")
            flat: true
            onClicked: chooser.waiting ? Keyboard.cancelCapture() : Keyboard.capture(chooser.role)
        }
    }

    background: null

    ScrollView {
        id: scroll
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: scroll.availableWidth
            spacing: 12

            Card {
                title: qsTr("Sound")
                Layout.fillWidth: true
                Layout.leftMargin: 24
                Layout.rightMargin: 24
                Layout.topMargin: 24

                GridLayout {
                    anchors.fill: parent
                    // One column on phones: labels can be long once translated
                    columns: root.width < 560 ? 1 : 2
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

            Card {
                title: qsTr("Key")
                Layout.fillWidth: true
                Layout.leftMargin: 24
                Layout.rightMargin: 24

                GridLayout {
                    anchors.fill: parent
                    columns: root.width < 560 ? 1 : 2
                    columnSpacing: 12

                    Label { text: qsTr("Keying") }
                    ComboBox {
                        model: [qsTr("Straight key"), qsTr("Iambic paddles")]
                        currentIndex: MorseSettings.keyerMode
                        onActivated: (index) => MorseSettings.keyerMode = index
                        implicitContentWidthPolicy: ComboBox.WidestText
                        Layout.fillWidth: true
                    }

                    Label { text: qsTr("Iambic mode"); enabled: MorseSettings.keyerMode === 1 }
                    ComboBox {
                        enabled: MorseSettings.keyerMode === 1
                        model: [qsTr("A: stops when the paddles are released"),
                                qsTr("B: sends one more element after a squeeze")]
                        currentIndex: MorseSettings.iambicMode
                        onActivated: (index) => MorseSettings.iambicMode = index
                        implicitContentWidthPolicy: ComboBox.WidestText
                        Layout.fillWidth: true
                    }

                    Label { text: qsTr("Reversed paddles"); enabled: MorseSettings.keyerMode === 1 }
                    Switch {
                        enabled: MorseSettings.keyerMode === 1
                        checked: MorseSettings.paddlesReversed
                        onToggled: MorseSettings.paddlesReversed = checked
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Dashes on the left contact, for left-handed operators")
                    }

                    // Keyboard keys: not on phones and tablets
                    Label { text: qsTr("Straight key on the keyboard"); visible: !Theme.mobile }
                    KeyChooser { role: "straight"; value: MorseSettings.straightKey; visible: !Theme.mobile }

                    Label { text: qsTr("Paddle keys"); enabled: MorseSettings.keyerMode === 1; visible: !Theme.mobile }
                    Segmented {
                        visible: !Theme.mobile
                        enabled: MorseSettings.keyerMode === 1
                        model: [qsTr("Right-handed"), qsTr("Left-handed"), qsTr("USB adapter")]
                        // Which ready-made choice the current keys are, if any
                        currentIndex: {
                            const keys = MorseSettings.dotKey + " " + MorseSettings.dashKey
                            const control = Qt.platform.os === "osx" ? Qt.Key_Meta : Qt.Key_Control
                            const alt = Qt.Key_Alt
                            if (keys === control + ":1 " + alt + ":1") return 0
                            if (keys === control + ":2 " + alt + ":2") return 1
                            if (keys === control + ":1 " + control + ":2") return 2
                            return -1
                        }
                        accessibleName: qsTr("Paddle keys")
                        onActivated: (index) => {
                            if (index === 0) Keyboard.useRightHandedKeys()
                            else if (index === 1) Keyboard.useLeftHandedKeys()
                            else Keyboard.useAdapterKeys()
                        }
                    }

                    Label { text: qsTr("Dot paddle key"); enabled: MorseSettings.keyerMode === 1; visible: !Theme.mobile }
                    KeyChooser { role: "dot"; value: MorseSettings.dotKey; enabled: MorseSettings.keyerMode === 1; visible: !Theme.mobile }

                    Label { text: qsTr("Dash paddle key"); enabled: MorseSettings.keyerMode === 1; visible: !Theme.mobile }
                    KeyChooser { role: "dash"; value: MorseSettings.dashKey; enabled: MorseSettings.keyerMode === 1; visible: !Theme.mobile }

                    Label { text: qsTr("Key that frees the captured mouse"); visible: !Theme.mobile }
                    KeyChooser { role: "release"; value: MorseSettings.releaseKey; visible: !Theme.mobile }

                    Label {
                        text: Theme.mobile ? qsTr("The keyer sends at the character speed.")
                                           : qsTr("The keyer sends at the character speed. Ctrl and Alt keys work wherever the focus is; a letter key does not key while you type in a text field.")
                        color: Theme.muted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        Layout.columnSpan: root.width < 560 ? 1 : 2
                    }
                }
            }

            Card {
                title: qsTr("Serial key")
                visible: Trainer.serialPorts().length > 0 || MorseSettings.serialEnabled
                Layout.fillWidth: true
                Layout.leftMargin: 24
                Layout.rightMargin: 24

                GridLayout {
                    anchors.fill: parent
                    // One column on phones: labels can be long once translated
                    columns: root.width < 560 ? 1 : 2
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

                    Label {
                        text: qsTr("Wiring: DTR and RTS feed the contacts. Straight key on CTS or DSR; paddles with dots on CTS or DCD and dashes on DSR.")
                        color: Theme.muted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        Layout.columnSpan: root.width < 560 ? 1 : 2
                    }
                }
            }

            Card {
                title: qsTr("Decoder")
                Layout.fillWidth: true
                Layout.leftMargin: 24
                Layout.rightMargin: 24

                ColumnLayout {
                    anchors.fill: parent
                    Switch {
                        text: qsTr("Adapts to the sender speed")
                        checked: MorseSettings.decoderAutoAdapt
                        onToggled: MorseSettings.decoderAutoAdapt = checked
                    }
                    Label {
                        text: qsTr("When off, the decoder expects the character speed.")
                        color: Theme.muted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    RowLayout {
                        enabled: MorseSettings.decoderAutoAdapt
                        Layout.fillWidth: true
                        Label { text: qsTr("Reactivity") }
                        ComboBox {
                            model: [qsTr("Fast"), qsTr("Normal"), qsTr("Stable")]
                            currentIndex: MorseSettings.decoderReactivity
                            onActivated: (index) => MorseSettings.decoderReactivity = index
                            Layout.fillWidth: true
                        }
                    }
                    Label {
                        text: qsTr("Fast follows speed changes sooner, Stable is less disturbed by clumsy keying. Large speed changes are always followed.")
                        color: Theme.muted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }

            Card {
                title: qsTr("Network")
                Layout.fillWidth: true
                Layout.leftMargin: 24
                Layout.rightMargin: 24

                GridLayout {
                    anchors.fill: parent
                    columns: root.width < 560 ? 1 : 2
                    columnSpacing: 12

                    Label { text: qsTr("STUN server") }
                    TextField {
                        text: MorseSettings.stunServer
                        placeholderText: "host:port"
                        onEditingFinished: MorseSettings.stunServer = text
                        Layout.fillWidth: true
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Tells your public address to find a path through your Internet box. Used for the next connection.")
                    }

                    Label { text: qsTr("Reception delay") }
                    SpinBox {
                        from: 0
                        to: 1000
                        stepSize: 50
                        value: MorseSettings.jitterDelayMs
                        textFromValue: (value) => qsTr("%1 ms").arg(value)
                        valueFromText: (text) => parseInt(text)
                        onValueModified: MorseSettings.jitterDelayMs = value
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Longer keeps the rhythm of the remote station on an irregular connection, shorter answers sooner.")
                    }
                }
            }

            Card {
                title: qsTr("Interface")
                Layout.fillWidth: true
                Layout.leftMargin: 24
                Layout.rightMargin: 24
                Layout.bottomMargin: 24

                GridLayout {
                    anchors.fill: parent
                    // One column on phones: labels can be long once translated
                    columns: root.width < 560 ? 1 : 2
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
