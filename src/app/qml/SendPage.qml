// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import EasyMorse

// Text to Morse: type a text and listen to it; in a room, the others hear it too
Page {
    id: root

    property string playedText
    property int playedIndex: -1

    background: null

    readonly property bool heard: Trainer.lobby.connectedCount > 0
                                  || Trainer.session.state === PeerSession.Connected
    readonly property string callsign: MorseSettings.callsign.length > 0 ? MorseSettings.callsign : "F4IKZ"
    // Ready-made phrases: label and text sent
    readonly property var phrases: [
        { label: "CQ", text: "CQ CQ DE " + callsign + " K" },
        { label: "QRZ?", text: "QRZ?" },
        { label: "RST 599", text: "UR RST 599 599" },
        { label: "73", text: "TNX FER QSO 73 DE " + callsign + " SK" },
        { label: "PARIS", text: "PARIS PARIS PARIS" }
    ]

    // Played text: sent characters dimmed, the one being sent highlighted
    function highlighted(text, index) {
        const escape = (s) => s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
        if (index < 0 || index >= text.length)
            return escape(text)
        return "<span style=\"color:" + Theme.muted + "\">" + escape(text.slice(0, index)) + "</span>"
             + "<span style=\"background-color:" + Theme.accent + "; color:" + Theme.accentInk + "\">"
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

    ScrollView {
        id: scroll
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: Math.min(scroll.availableWidth, 900)
            x: 0
            spacing: 20

            Label {
                text: qsTr("Text to send").toUpperCase()
                color: Theme.muted
                font.pixelSize: 12
                font.weight: Font.DemiBold
                font.letterSpacing: 1.2
                Layout.topMargin: 24
                Layout.leftMargin: 24
            }
            RowLayout {
                spacing: 12
                Layout.fillWidth: true
                Layout.leftMargin: 24
                Layout.rightMargin: 24
                Layout.topMargin: -12

                TextField {
                    id: message
                    text: "CQ CQ DE " + root.callsign + " K"
                    font.family: Theme.monoFont
                    font.pixelSize: 18
                    Layout.fillWidth: true
                    onAccepted: root.playOrStop()
                    Accessible.name: qsTr("Text to send")
                }
                Button {
                    text: Trainer.playing ? qsTr("Stop") : qsTr("Play")
                    icon.source: Trainer.playing ? "../icons/stop.svg" : "../icons/play.svg"
                    icon.width: 18
                    icon.height: 18
                    Material.background: Theme.accent
                    Material.foreground: Theme.accentInk
                    font.weight: Font.Bold
                    enabled: Trainer.playing || (message.text.length > 0 && Trainer.audioRunning)
                    onClicked: root.playOrStop()
                    Layout.preferredHeight: 56
                }
            }
            Flow {
                spacing: 8
                Layout.fillWidth: true
                Layout.leftMargin: 24
                Layout.rightMargin: 24
                Layout.topMargin: -8

                Repeater {
                    model: root.phrases
                    Button {
                        required property var modelData
                        text: modelData.label
                        flat: true
                        font.family: Theme.monoFont
                        font.pixelSize: 13
                        enabled: !Trainer.playing
                        onClicked: message.text = modelData.text
                        ToolTip.visible: hovered
                        ToolTip.text: modelData.text
                        background: Rectangle {
                            implicitHeight: 36
                            radius: 18
                            color: parent.hovered ? Theme.raised : "transparent"
                            border.color: Theme.lineStrong
                        }
                    }
                }
            }

            Card {
                title: Trainer.playing ? (root.heard ? qsTr("Sending · heard by your correspondents") : qsTr("Sending"))
                                       : qsTr("Last text")
                Layout.fillWidth: true
                Layout.leftMargin: 24
                Layout.rightMargin: 24

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 16

                    Label {
                        text: root.playedText.length > 0 ? root.highlighted(root.playedText.toUpperCase(), root.playedIndex)
                                                         : qsTr("Type a text or choose a phrase, then play it.")
                        // Rich text: styled text has no background color
                        textFormat: root.playedText.length > 0 ? Text.RichText : Text.PlainText
                        // Morse text may break anywhere, sentences between words
                        wrapMode: root.playedText.length > 0 ? Text.WrapAnywhere : Text.WordWrap
                        font.family: root.playedText.length > 0 ? Theme.monoFont : Theme.uiFont
                        font.pixelSize: root.playedText.length > 0 ? 34 : 14
                        font.letterSpacing: root.playedText.length > 0 ? 3 : 0
                        color: root.playedText.length > 0 ? Theme.text : Theme.muted
                        Layout.fillWidth: true
                    }
                    ProgressBar {
                        visible: Trainer.playing
                        from: 0
                        to: Math.max(1, root.playedText.length)
                        value: root.playedIndex + 1
                        Layout.fillWidth: true
                    }
                }
            }

            Label {
                text: Theme.mobile ? qsTr("Use the Key page to answer.")
                                   : qsTr("Hold the Alt key or use the Key page to answer.")
                color: Theme.muted
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.leftMargin: 24
                Layout.rightMargin: 24
                Layout.bottomMargin: 24
            }
        }
    }
}
