// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import EasyMorse

// Morse with a remote station over the Internet
Page {
    id: root

    readonly property var session: Trainer.session
    readonly property bool connected: session.state === PeerSession.Connected
    readonly property bool busy: session.state === PeerSession.Preparing
                                 || session.state === PeerSession.Inviting
                                 || session.state === PeerSession.Connecting

    // Received Morse: finished messages, and the one in progress
    property string receivedText
    property string currentText
    property bool revealed: false

    function stateText() {
        switch (session.state) {
        case PeerSession.Preparing: return qsTr("Preparing your invitation…")
        case PeerSession.Inviting: return qsTr("Waiting for your correspondent's invitation")
        case PeerSession.Connecting: return qsTr("Connecting…")
        case PeerSession.Connected: return qsTr("Connected with %1").arg(session.peerCallsign || qsTr("your correspondent"))
        case PeerSession.Disconnected: return qsTr("Disconnected")
        case PeerSession.Failed: return qsTr("No connection possible between your two networks")
        default: return qsTr("Not connected")
        }
    }

    function pathText() {
        switch (session.pathType) {
        case "host": return qsTr("direct, same local network")
        case "srflx": return qsTr("direct, through the Internet")
        case "relay": return qsTr("through a relay server")
        default: return ""
        }
    }

    Connections {
        target: Trainer
        function onRemoteCharDecoded(pText, pPattern) {
            root.currentText += pText.length > 0 ? pText : "[" + pPattern + "]"
        }
        function onRemoteWordGap() { root.currentText += " " }
        function onRemoteActiveChanged() {
            // Message over: it joins the finished ones
            if (!Trainer.remoteActive && root.currentText.length > 0) {
                root.receivedText += root.currentText.trim() + "\n"
                root.currentText = ""
                root.revealed = false
            }
        }
    }

    Connections {
        target: root.session
        function onTextReceived(pText) {
            messages.append({ from: root.session.peerCallsign || qsTr("Correspondent"), message: pText })
        }
    }

    ListModel { id: messages }

    ScrollView {
        id: scroll
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: scroll.availableWidth
            spacing: 12

            GroupBox {
                title: qsTr("Connection")
                Layout.fillWidth: true
                Layout.margins: 12

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("Your callsign") }
                        TextField {
                            text: MorseSettings.callsign
                            placeholderText: qsTr("Callsign or first name")
                            enabled: !root.busy && !root.connected
                            onEditingFinished: MorseSettings.callsign = text
                            Layout.fillWidth: true
                        }
                    }

                    Label {
                        text: root.stateText()
                        font.bold: true
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Label {
                        visible: !root.connected
                        text: qsTr("1. Prepare your invitation and send it to your correspondent (mail, chat…).\n2. Paste the invitation they send you and connect. Each of you does both steps.")
                        wrapMode: Text.WordWrap
                        opacity: 0.7
                        Layout.fillWidth: true
                    }

                    Button {
                        visible: !root.busy && !root.connected
                        text: qsTr("Prepare my invitation")
                        onClicked: root.session.start()
                    }

                    // Our invitation, to copy
                    RowLayout {
                        visible: root.session.invitation.length > 0 && !root.connected
                        Layout.fillWidth: true
                        TextArea {
                            id: myInvitation
                            text: root.session.invitation
                            readOnly: true
                            wrapMode: TextEdit.WrapAnywhere
                            font.family: "monospace"
                            font.pixelSize: 11
                            Layout.fillWidth: true
                            Layout.maximumHeight: 90
                        }
                        Button {
                            text: qsTr("Copy")
                            onClicked: {
                                myInvitation.selectAll()
                                myInvitation.copy()
                                myInvitation.deselect()
                            }
                        }
                    }

                    // The correspondent's invitation, to paste
                    RowLayout {
                        visible: !root.connected
                        Layout.fillWidth: true
                        TextArea {
                            id: peerInvitation
                            placeholderText: qsTr("Paste your correspondent's invitation here")
                            wrapMode: TextEdit.WrapAnywhere
                            font.family: "monospace"
                            font.pixelSize: 11
                            Layout.fillWidth: true
                            Layout.maximumHeight: 90
                        }
                        ColumnLayout {
                            Button {
                                text: qsTr("Paste")
                                onClicked: { peerInvitation.clear(); peerInvitation.paste() }
                            }
                            Button {
                                text: qsTr("Connect")
                                highlighted: true
                                enabled: peerInvitation.text.trim().length > 0
                                onClicked: {
                                    joinError.text = root.session.join(peerInvitation.text)
                                    if (joinError.text.length === 0)
                                        peerInvitation.clear()
                                }
                            }
                        }
                    }
                    Label {
                        id: joinError
                        visible: text.length > 0 && !root.connected
                        color: Material.color(Material.Red)
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    // Connected
                    GridLayout {
                        visible: root.connected
                        columns: 2
                        columnSpacing: 12
                        Layout.fillWidth: true

                        Label { text: qsTr("Path") }
                        Label { text: root.pathText() }
                        Label { text: qsTr("Round trip") }
                        Label { text: root.session.roundTripMs >= 0 ? qsTr("%1 ms").arg(root.session.roundTripMs) : "…" }
                        Label { text: qsTr("Security code") }
                        Label {
                            text: root.session.fingerprint
                            font.family: "monospace"
                            font.bold: true
                        }
                    }
                    Label {
                        visible: root.connected
                        text: qsTr("Read the security code to each other: if it differs, someone is in the middle.")
                        wrapMode: Text.WordWrap
                        opacity: 0.7
                        Layout.fillWidth: true
                    }
                    Button {
                        visible: root.connected || root.busy
                        text: root.connected ? qsTr("Disconnect") : qsTr("Cancel")
                        onClicked: root.session.stop()
                    }
                }
            }

            GroupBox {
                title: qsTr("Reception")
                Layout.fillWidth: true
                Layout.leftMargin: 12
                Layout.rightMargin: 12

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        Rectangle {
                            // Lit while the correspondent's key is down
                            width: 18; height: 18; radius: 9
                            color: Trainer.remoteKeyDown ? Material.accent : Material.color(Material.Grey)
                        }
                        Label {
                            text: root.connected ? qsTr("Correspondent at %1 WPM").arg(Math.round(Trainer.remoteDecoderSpeed)) : qsTr("Nothing received yet")
                            Layout.fillWidth: true
                        }
                        Label { text: qsTr("Show") }
                        ComboBox {
                            model: [qsTr("Nothing, listen"), qsTr("After the message"), qsTr("Live")]
                            currentIndex: MorseSettings.receiveDecodeMode
                            onActivated: (index) => MorseSettings.receiveDecodeMode = index
                            implicitContentWidthPolicy: ComboBox.WidestText
                        }
                    }

                    TextArea {
                        readOnly: true
                        wrapMode: TextEdit.Wrap
                        font.pixelSize: 20
                        font.family: "monospace"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 140
                        text: {
                            const mode = MorseSettings.receiveDecodeMode
                            if (mode === 0)
                                return ""
                            if (mode === 2 || root.revealed)
                                return root.receivedText + root.currentText
                            return root.receivedText + (root.currentText.length > 0 ? "…" : "")
                        }
                        onTextChanged: cursorPosition = length

                        Label {
                            anchors.centerIn: parent
                            visible: MorseSettings.receiveDecodeMode === 0
                            width: parent.width - 32
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                            text: qsTr("Listen and copy what you hear. Choose what to show above to check yourself.")
                            opacity: 0.5
                            font: Qt.application.font
                        }
                    }
                    RowLayout {
                        Button {
                            visible: MorseSettings.receiveDecodeMode === 1 && root.currentText.length > 0 && !root.revealed
                            text: qsTr("Reveal now")
                            onClicked: root.revealed = true
                        }
                        Button {
                            text: qsTr("Clear")
                            flat: true
                            onClicked: { root.receivedText = ""; root.currentText = "" }
                        }
                    }
                }
            }

            GroupBox {
                title: qsTr("Messages")
                Layout.fillWidth: true
                Layout.leftMargin: 12
                Layout.rightMargin: 12
                Layout.bottomMargin: 12

                ColumnLayout {
                    anchors.fill: parent

                    Repeater {
                        model: messages
                        Label {
                            required property string from
                            required property string message
                            text: "<b>" + from.replace(/</g, "&lt;") + "</b> " + message.replace(/</g, "&lt;")
                            textFormat: Text.StyledText
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        TextField {
                            id: messageField
                            placeholderText: qsTr("Text message")
                            enabled: root.connected
                            Layout.fillWidth: true
                            onAccepted: sendButton.clicked()
                        }
                        Button {
                            id: sendButton
                            text: qsTr("Send")
                            enabled: root.connected && messageField.text.length > 0
                            onClicked: {
                                if (root.session.sendText(messageField.text)) {
                                    messages.append({ from: MorseSettings.callsign || qsTr("Me"), message: messageField.text })
                                    messageField.clear()
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
