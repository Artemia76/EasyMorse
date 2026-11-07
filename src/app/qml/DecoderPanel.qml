// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import EasyMorse

// Decoded Morse at the bottom of the window, whatever the page: one line for
// what we send (keyed or played), one per correspondent connected
Pane {
    id: root

    // Kept per line, the end is shown
    readonly property int maxChars: 400
    // Manual session and members of the room; a line shows once connected
    readonly property var sessions: [Trainer.session].concat(Trainer.lobby.peers)
    property int connectedCount: 0
    readonly property int nameWidth: root.width < 520 ? 72 : 104
    readonly property int textSize: root.width < 520 ? 17 : 20

    signal clearRequested()

    function append(text, more) {
        const all = text + more
        return all.length > root.maxChars ? all.slice(all.length - root.maxChars) : all
    }

    // Set apart from the pages, like a reception tape
    background: Rectangle {
        color: Theme.tape
        Rectangle {
            width: parent.width
            height: 1
            color: Theme.lineStrong
        }
    }
    leftPadding: 16
    rightPadding: 16
    topPadding: 8
    bottomPadding: 12

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Decoded").toUpperCase()
                color: Theme.muted
                font.pixelSize: 12
                font.weight: Font.DemiBold
                font.letterSpacing: 1.2
                Layout.fillWidth: true
            }
            // What to show of the correspondents: nothing, after the message, live
            Segmented {
                visible: root.connectedCount > 0
                model: root.width < 520 ? [qsTr("Listen"), qsTr("After"), qsTr("Live")]
                                        : [qsTr("Listen"), qsTr("After the message"), qsTr("Live")]
                currentIndex: MorseSettings.receiveDecodeMode
                accessibleName: qsTr("Show the correspondents")
                onActivated: (index) => MorseSettings.receiveDecodeMode = index
            }
            Button {
                text: qsTr("Clear")
                flat: true
                font.pixelSize: 13
                Material.foreground: Theme.accentText
                Layout.preferredHeight: 40
                onClicked: root.clearRequested()
            }
        }

        // ---- Our own line: keyed and played text
        RowLayout {
            id: own
            property string text
            Layout.fillWidth: true
            spacing: 8

            Connections {
                target: Trainer
                function onCharDecoded(pText, pPattern) {
                    own.text = root.append(own.text, pText.length > 0 ? pText : "[" + pPattern + "]")
                }
                function onWordGap() { own.text = root.append(own.text, " ") }
                function onCharSent(pText) { own.text = root.append(own.text, pText) }
                function onPlayingChanged() {
                    // A played text is a word of its own
                    if (own.text.length > 0 && !own.text.endsWith(" "))
                        own.text = root.append(own.text, " ")
                }
            }
            Connections {
                target: root
                function onClearRequested() { own.text = "" }
            }

            Rectangle {
                width: 10; height: 10; radius: 5
                color: Trainer.keyDown || Trainer.playing ? Theme.accent : Theme.lineStrong
            }
            Label {
                text: MorseSettings.callsign.length > 0 ? MorseSettings.callsign : qsTr("Me")
                font.weight: Font.Bold
                elide: Text.ElideRight
                Layout.preferredWidth: root.nameWidth
            }
            Label {
                // The end of the line stays visible, the pattern being keyed after it
                text: own.text.replace(/&/g, "&amp;").replace(/</g, "&lt;") + (Trainer.decoderPattern.length > 0
                                  ? " <font color=\"" + Theme.accentText + "\">"
                                    + Theme.pattern(Trainer.decoderPattern) + "</font>" : "")
                textFormat: Text.StyledText
                font.family: Theme.monoFont
                font.pixelSize: root.textSize
                elide: Text.ElideLeft
                horizontalAlignment: Text.AlignRight
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("%1 WPM").arg(Math.round(Trainer.decoderSpeed))
                color: Theme.muted
                font.family: Theme.monoFont
                font.pixelSize: 13
                horizontalAlignment: Text.AlignRight
                visible: root.width >= 520
                Layout.preferredWidth: 84
            }
        }

        // ---- One line per correspondent
        Repeater {
            model: root.sessions

            RowLayout {
                id: line
                required property var modelData
                readonly property bool connected: modelData.state === PeerSession.Connected
                // Finished messages and the one being received
                property string done
                property string current
                property bool revealed: false
                property real wpm: 0

                function finish() {
                    if (line.current.length > 0)
                        line.done = root.append(line.done, line.current.trim() + " ")
                    line.current = ""
                    line.revealed = false
                }

                visible: connected
                onConnectedChanged: {
                    root.connectedCount += connected ? 1 : -1
                    if (!connected)
                        finish()
                }
                Component.onCompleted: if (connected) root.connectedCount += 1
                Component.onDestruction: if (connected) root.connectedCount -= 1
                Layout.fillWidth: true
                spacing: 8

                Connections {
                    target: Trainer
                    function onRemoteCharDecoded(pSession, pText, pPattern) {
                        if (pSession === line.modelData)
                            line.current += pText.length > 0 ? pText : "[" + pPattern + "]"
                    }
                    function onRemoteWordGap(pSession) {
                        if (pSession === line.modelData)
                            line.current += " "
                    }
                    function onRemoteSpeedChanged(pSession) {
                        if (pSession === line.modelData)
                            line.wpm = Trainer.stationSpeed(pSession)
                    }
                }
                Connections {
                    target: line.modelData.remoteKey
                    function onKeyChanged(pDown) {
                        // The message is over after a few seconds without keying
                        if (pDown)
                            messageEnd.stop()
                        else
                            messageEnd.restart()
                    }
                }
                Connections {
                    target: root
                    function onClearRequested() { line.done = ""; line.current = "" }
                }
                Timer {
                    id: messageEnd
                    interval: 3000
                    onTriggered: line.finish()
                }

                Rectangle {
                    width: 10; height: 10; radius: 5
                    color: line.modelData.remoteKey.down ? Theme.ok : Theme.lineStrong
                }
                Label {
                    text: line.modelData.peerCallsign.length > 0 ? line.modelData.peerCallsign : qsTr("Correspondent")
                    color: Theme.okText
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                    Layout.preferredWidth: root.nameWidth
                }
                Label {
                    text: {
                        const mode = MorseSettings.receiveDecodeMode
                        if (mode === 0)
                            return line.modelData.remoteKey.down || messageEnd.running ? qsTr("listening…") : ""
                        if (mode === 2 || line.revealed)
                            return line.done + line.current
                        return line.done + (line.current.length > 0 ? "…" : "")
                    }
                    color: MorseSettings.receiveDecodeMode === 0 ? Theme.muted : Theme.textSoft
                    font.family: MorseSettings.receiveDecodeMode === 0 ? Theme.uiFont : Theme.monoFont
                    font.pixelSize: MorseSettings.receiveDecodeMode === 0 ? 14 : root.textSize
                    elide: Text.ElideLeft
                    horizontalAlignment: Text.AlignRight
                    Layout.fillWidth: true

                    TapHandler {
                        // After the message mode: show the message now
                        enabled: MorseSettings.receiveDecodeMode === 1 && line.current.length > 0
                        onTapped: line.revealed = true
                    }
                }
                Label {
                    text: line.wpm > 0 ? qsTr("%1 WPM").arg(Math.round(line.wpm)) : ""
                    color: Theme.muted
                    font.family: Theme.monoFont
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignRight
                    visible: root.width >= 520
                    Layout.preferredWidth: 84
                }
            }
        }
    }
}
