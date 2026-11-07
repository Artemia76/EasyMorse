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
    // In a room of the server the sessions open by themselves; otherwise
    // invitations are exchanged by hand
    readonly property bool inRoom: Trainer.lobby.client.state === SignalingClient.Online
                                   && Trainer.lobby.client.room.length > 0
    readonly property bool manual: !inRoom || root.connected || root.busy
    readonly property bool busy: session.state === PeerSession.Preparing
                                 || session.state === PeerSession.Inviting
                                 || session.state === PeerSession.Connecting

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
        target: root.session
        function onTextReceived(pText) {
            messages.append({ from: root.session.peerCallsign || qsTr("Correspondent"), message: pText })
        }
    }

    ListModel { id: messages }

    background: null

    ScrollView {
        id: scroll
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: scroll.availableWidth
            spacing: 16

            ServerPanel {
                Layout.fillWidth: true
                Layout.leftMargin: 24
                Layout.rightMargin: 24
                Layout.topMargin: 24
            }

            Card {
                title: qsTr("Connection")
                visible: root.manual
                Layout.fillWidth: true
                Layout.leftMargin: 24
                Layout.rightMargin: 24

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    Label {
                        text: root.stateText()
                        font.bold: true
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Label {
                        visible: !root.connected && root.manual
                        text: qsTr("1. Prepare your invitation and send it to your correspondent (mail, chat…).\n2. Paste the invitation they send you and connect. Each of you does both steps.")
                        wrapMode: Text.WordWrap
                        color: Theme.muted
                        Layout.fillWidth: true
                    }

                    Button {
                        visible: !root.busy && !root.connected && root.manual
                        text: qsTr("Prepare my invitation")
                        onClicked: root.session.start()
                    }

                    // Our invitation, to copy
                    RowLayout {
                        visible: root.session.invitation.length > 0 && !root.connected && root.manual
                        Layout.fillWidth: true
                        TextArea {
                            id: myInvitation
                            text: root.session.invitation
                            readOnly: true
                            wrapMode: TextEdit.WrapAnywhere
                            font.family: Theme.monoFont
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
                        visible: !root.connected && root.manual
                        Layout.fillWidth: true
                        TextArea {
                            id: peerInvitation
                            placeholderText: qsTr("Paste your correspondent's invitation here")
                            wrapMode: TextEdit.WrapAnywhere
                            font.family: Theme.monoFont
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
                        color: Theme.error
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
                            font.family: Theme.monoFont
                            font.bold: true
                        }
                    }
                    Label {
                        visible: root.connected
                        text: qsTr("Read the security code to each other: if it differs, someone is in the middle.")
                        wrapMode: Text.WordWrap
                        color: Theme.muted
                        Layout.fillWidth: true
                    }
                    Button {
                        visible: (root.connected || root.busy) && root.manual
                        text: root.connected ? qsTr("Disconnect") : qsTr("Cancel")
                        onClicked: root.session.stop()
                    }
                }
            }

            // Text messages of the manual session (a room has its chat)
            Card {
                title: qsTr("Messages")
                visible: root.manual
                Layout.fillWidth: true
                Layout.leftMargin: 24
                Layout.rightMargin: 24
                Layout.bottomMargin: 24

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
