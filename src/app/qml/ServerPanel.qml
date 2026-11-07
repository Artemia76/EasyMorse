// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import EasyMorse

// Match-making server: account, rooms, members to call, room chat
GroupBox {
    id: root

    readonly property var lobby: Trainer.lobby
    readonly property var client: Trainer.lobby.client
    readonly property bool online: client.state === SignalingClient.Online
    readonly property bool busy: client.state === SignalingClient.LoggingIn
                                 || client.state === SignalingClient.Connecting
    readonly property bool sessionOpen: Trainer.session.state !== PeerSession.Idle
                                        && Trainer.session.state !== PeerSession.Disconnected
                                        && Trainer.session.state !== PeerSession.Failed

    title: qsTr("Server")

    function failureText(reason) {
        switch (reason) {
        case "declined": return qsTr("%1 declined the call")
        case "busy": return qsTr("%1 is busy")
        case "left": return qsTr("%1 left the room")
        default: return qsTr("%1 did not answer")
        }
    }

    Connections {
        target: root.client
        function onChatReceived(pFrom, pText) { roomChat.append({ from: pFrom, message: pText }) }
        function onRoomChanged() { if (root.client.room.length === 0) roomChat.clear() }
        function onStateChanged() { if (root.online) root.client.refreshRooms() }
    }
    Connections {
        target: root.lobby
        function onCallFailed(pPeer, pReason) { callMessage.text = root.failureText(pReason).arg(pPeer) }
        function onCallStateChanged() { if (root.lobby.callState !== Lobby.Idle) callMessage.text = "" }
    }

    ListModel { id: roomChat }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        // ---- Logged out: account
        GridLayout {
            visible: !root.online
            columns: root.width < 520 ? 1 : 2
            columnSpacing: 12
            Layout.fillWidth: true

            Label { text: qsTr("Server address") }
            TextField {
                text: MorseSettings.serverUrl
                placeholderText: "morse.example.org"
                inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoAutoUppercase
                enabled: !root.busy
                onEditingFinished: MorseSettings.serverUrl = text
                Layout.fillWidth: true
            }
            Label { text: qsTr("Callsign") }
            TextField {
                id: loginCallsign
                // Also the station callsign of the manual invitations
                text: MorseSettings.callsign
                placeholderText: qsTr("Callsign or first name")
                enabled: !root.busy && !root.sessionOpen
                onEditingFinished: MorseSettings.callsign = text
                Layout.fillWidth: true
            }
            Label { text: qsTr("Password") }
            TextField {
                id: loginPassword
                echoMode: TextInput.Password
                placeholderText: qsTr("At least 8 characters")
                enabled: !root.busy
                Layout.fillWidth: true
                onAccepted: loginButton.clicked()
            }
        }
        RowLayout {
            visible: !root.online
            Button {
                id: loginButton
                text: root.busy ? qsTr("Connecting…") : qsTr("Log in")
                highlighted: true
                enabled: !root.busy && MorseSettings.serverUrl.length > 0 && loginPassword.text.length > 0
                onClicked: {
                    root.client.login(loginCallsign.text, loginPassword.text)
                    loginPassword.clear()
                }
            }
            Button {
                text: qsTr("Create account")
                enabled: !root.busy && MorseSettings.serverUrl.length > 0 && loginPassword.text.length > 0
                onClicked: {
                    root.client.registerAccount(loginCallsign.text, loginPassword.text)
                    loginPassword.clear()
                }
            }
        }
        Label {
            visible: !root.online && root.client.lastError.length > 0
            text: root.client.lastError
            color: Material.color(Material.Red)
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Label {
            visible: !root.online
            text: qsTr("No server? Exchange invitations with your correspondent below.")
            opacity: 0.7
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        // ---- Logged in: rooms and members
        RowLayout {
            visible: root.online
            Layout.fillWidth: true
            Label {
                text: qsTr("Logged in as %1").arg(root.client.callsign)
                font.bold: true
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("Log out")
                flat: true
                onClicked: root.client.logout()
            }
        }
        RowLayout {
            visible: root.online
            Layout.fillWidth: true
            ComboBox {
                id: roomBox
                editable: true
                model: root.client.rooms.map((room) => room.name)
                Layout.fillWidth: true
                Component.onCompleted: editText = ""
            }
            Button {
                text: qsTr("Join")
                enabled: roomBox.editText.trim().length > 0
                onClicked: root.client.joinRoom(roomBox.editText.trim())
            }
            Button {
                visible: root.client.room.length > 0
                text: qsTr("Leave")
                flat: true
                onClicked: root.client.leaveRoom()
            }
        }
        Label {
            visible: root.online
            text: root.client.room.length > 0 ? qsTr("Room %1").arg(root.client.room)
                                              : qsTr("Choose or type a room, then join it")
            opacity: root.client.room.length > 0 ? 1 : 0.7
            font.bold: root.client.room.length > 0
        }

        // Members, with a call button
        Repeater {
            model: root.online ? root.client.members : []
            RowLayout {
                required property string modelData
                Layout.fillWidth: true
                Label {
                    text: parent.modelData === root.client.callsign ? parent.modelData + " " + qsTr("(you)") : parent.modelData
                    Layout.fillWidth: true
                }
                Button {
                    visible: parent.modelData !== root.client.callsign
                    text: qsTr("Call")
                    enabled: root.lobby.callState === Lobby.Idle
                    onClicked: root.lobby.call(parent.modelData)
                }
            }
        }

        // Call in progress
        RowLayout {
            visible: root.lobby.callState === Lobby.Calling
            Layout.fillWidth: true
            BusyIndicator { running: parent.visible; implicitWidth: 32; implicitHeight: 32 }
            Label { text: qsTr("Calling %1…").arg(root.lobby.callPeer); Layout.fillWidth: true }
            Button { text: qsTr("Cancel"); onClicked: root.lobby.hangup() }
        }
        RowLayout {
            visible: root.lobby.callState === Lobby.InCall
            Layout.fillWidth: true
            Label { text: qsTr("In a session with %1").arg(root.lobby.callPeer); font.bold: true; Layout.fillWidth: true }
            Button { text: qsTr("Hang up"); onClicked: root.lobby.hangup() }
        }
        Label {
            id: callMessage
            visible: text.length > 0
            color: Material.color(Material.Orange)
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        // Room chat
        Repeater {
            model: roomChat
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
            visible: root.online && root.client.room.length > 0
            Layout.fillWidth: true
            TextField {
                id: roomMessage
                placeholderText: qsTr("Message to the room")
                Layout.fillWidth: true
                onAccepted: roomSend.clicked()
            }
            Button {
                id: roomSend
                text: qsTr("Send")
                enabled: roomMessage.text.trim().length > 0
                onClicked: { root.client.sendChat(roomMessage.text); roomMessage.clear() }
            }
        }
    }
}
