// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import EasyMorse

// Match-making server: account, room, members (each one connected peer to
// peer by itself) and room chat
ColumnLayout {
    id: root

    readonly property var lobby: Trainer.lobby
    readonly property var client: Trainer.lobby.client
    readonly property bool online: client.state === SignalingClient.Online
    readonly property bool busy: client.state === SignalingClient.LoggingIn
                                 || client.state === SignalingClient.Connecting
    readonly property bool inRoom: online && client.room.length > 0
    readonly property bool sessionOpen: Trainer.session.state !== PeerSession.Idle
                                        && Trainer.session.state !== PeerSession.Disconnected
                                        && Trainer.session.state !== PeerSession.Failed
    // Chat beside the members when there is room for it
    readonly property bool twoColumns: width >= 760

    spacing: 16

    function linkText(session) {
        if (!session)
            return ""
        switch (session.state) {
        case PeerSession.Connected:
            return (session.pathType === "relay" ? qsTr("through the relay") : qsTr("direct"))
                   + (session.roundTripMs >= 0 ? " · " + qsTr("%1 ms").arg(session.roundTripMs) : "")
        case PeerSession.Disconnected:
        case PeerSession.Failed:
            return qsTr("no connection, trying again…")
        default:
            return qsTr("connecting…")
        }
    }

    Connections {
        target: root.client
        function onChatReceived(pFrom, pText) { roomChat.append({ from: pFrom, message: pText }) }
        function onRoomChanged() { if (root.client.room.length === 0) roomChat.clear() }
        function onStateChanged() { if (root.online) root.client.refreshRooms() }
    }

    ListModel { id: roomChat }

    // ---- Logged out: account
    Card {
        visible: !root.online
        title: qsTr("Server")
        Layout.fillWidth: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            GridLayout {
                columns: root.width < 520 ? 1 : 2
                columnSpacing: 16
                rowSpacing: 8
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
                    font.family: Theme.monoFont
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
            Flow {
                spacing: 8
                Layout.fillWidth: true
                Button {
                    id: loginButton
                    text: root.busy ? qsTr("Connecting…") : qsTr("Log in")
                    Material.background: Theme.accent
                    Material.foreground: Theme.accentInk
                    enabled: !root.busy && MorseSettings.serverUrl.length > 0 && loginPassword.text.length > 0
                    onClicked: {
                        root.client.login(loginCallsign.text, loginPassword.text)
                        loginPassword.clear()
                    }
                }
                Button {
                    text: qsTr("Create account")
                    flat: true
                    enabled: !root.busy && MorseSettings.serverUrl.length > 0 && loginPassword.text.length > 0
                    onClicked: {
                        root.client.registerAccount(loginCallsign.text, loginPassword.text)
                        loginPassword.clear()
                    }
                }
            }
            Label {
                visible: root.client.lastError.length > 0
                text: root.client.lastError
                color: Theme.error
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("No server? Exchange invitations with your correspondent below.")
                color: Theme.muted
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }

    // ---- Logged in: room
    RowLayout {
        visible: root.online
        Layout.fillWidth: true
        spacing: 12

        ColumnLayout {
            spacing: 2
            Layout.fillWidth: true
            Label {
                text: qsTr("Logged in as %1").arg(root.client.callsign).toUpperCase()
                color: Theme.muted
                font.pixelSize: 12
                font.weight: Font.DemiBold
                font.letterSpacing: 1.2
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Label {
                text: root.inRoom ? qsTr("Room %1").arg(root.client.room) : qsTr("Choose a room")
                font.pixelSize: 26
                font.weight: Font.Bold
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
        Button {
            text: qsTr("Log out")
            flat: true
            onClicked: root.client.logout()
        }
    }
    Flow {
        visible: root.online
        spacing: 8
        Layout.fillWidth: true
        ComboBox {
            id: roomBox
            editable: true
            width: Math.min(320, root.width)
            model: root.client.rooms.map((room) => room.name)
            Component.onCompleted: editText = ""
            Accessible.name: qsTr("Room")
        }
        Button {
            text: qsTr("Join")
            Material.background: Theme.accent
            Material.foreground: Theme.accentInk
            enabled: roomBox.editText.trim().length > 0
            onClicked: root.client.joinRoom(roomBox.editText.trim())
        }
        Button {
            visible: root.inRoom
            text: qsTr("Leave")
            flat: true
            onClicked: root.client.leaveRoom()
        }
    }
    Label {
        visible: root.online
        text: !root.inRoom ? qsTr("Type a room name or choose one, then join it.")
              : root.client.members.length > 1 ? qsTr("You hear the members of the room and they hear you: key when you like.")
              : qsTr("Nobody else in this room yet.")
        color: Theme.muted
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }

    GridLayout {
        visible: root.inRoom
        columns: root.twoColumns ? 2 : 1
        columnSpacing: 16
        rowSpacing: 16
        Layout.fillWidth: true

        // Members and the state of the link with each of them
        ColumnLayout {
            spacing: 10
            Layout.fillWidth: true
            Layout.preferredWidth: 2
            Layout.alignment: Qt.AlignTop

            Repeater {
                model: root.inRoom ? root.client.members : []

                Rectangle {
                    id: member
                    required property string modelData
                    readonly property bool self: modelData === root.client.callsign
                    // Re-read when the list of sessions changes
                    readonly property var session: { root.lobby.peers; return self ? null : root.lobby.peer(modelData) }
                    readonly property bool connected: session !== null && session.state === PeerSession.Connected

                    Layout.fillWidth: true
                    implicitHeight: memberRow.implicitHeight + 28
                    radius: Theme.radius
                    color: member.self || member.connected ? Theme.surface : "transparent"
                    border.width: member.self || member.connected ? 0 : 1
                    border.color: Theme.lineStrong

                    GridLayout {
                        id: memberRow
                        anchors.fill: parent
                        anchors.margins: 14
                        columns: root.width < 520 ? 2 : 3
                        columnSpacing: 14
                        rowSpacing: 6

                        Rectangle {
                            width: 40; height: 40; radius: 20
                            color: member.self ? Theme.raised : member.connected ? Theme.okBackground : Theme.raised
                            Label {
                                anchors.centerIn: parent
                                text: member.modelData.slice(0, 2)
                                font.weight: Font.Bold
                                color: member.self ? Theme.accentText : member.connected ? Theme.okText : Theme.muted
                            }
                        }
                        ColumnLayout {
                            spacing: 2
                            Layout.fillWidth: true
                            Label {
                                textFormat: Text.StyledText
                                text: "<b>" + member.modelData + "</b>"
                                      + (member.self ? " <font color=\"" + Theme.muted + "\">" + qsTr("(you)") + "</font>" : "")
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            RowLayout {
                                visible: !member.self
                                spacing: 6
                                Rectangle {
                                    visible: member.connected
                                    width: 8; height: 8; radius: 4
                                    color: Theme.ok
                                }
                                Label {
                                    text: root.linkText(member.session)
                                    color: member.connected ? Theme.okText : Theme.muted
                                    font.pixelSize: 13
                                }
                            }
                        }
                        ColumnLayout {
                            // Security code of the link: the same on both sides
                            visible: member.connected
                            spacing: 0
                            Layout.columnSpan: root.width < 520 ? 2 : 1
                            Layout.alignment: root.width < 520 ? Qt.AlignLeft : Qt.AlignRight
                            Label {
                                text: qsTr("Security code").toUpperCase()
                                color: Theme.muted
                                font.pixelSize: 11
                                font.letterSpacing: 1
                                Layout.alignment: root.width < 520 ? Qt.AlignLeft : Qt.AlignRight
                            }
                            Label {
                                text: member.session ? member.session.fingerprint : ""
                                font.family: Theme.monoFont
                                font.pixelSize: 15
                                Layout.alignment: root.width < 520 ? Qt.AlignLeft : Qt.AlignRight
                                ToolTip.visible: codeArea.containsMouse
                                ToolTip.text: qsTr("Security code: %1 sees the same one, unless someone is in the middle.").arg(member.modelData)
                                MouseArea { id: codeArea; anchors.fill: parent; hoverEnabled: true }
                            }
                        }
                    }
                }
            }
        }

        // Room chat
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            Layout.alignment: Qt.AlignTop
            implicitHeight: chat.implicitHeight + 32
            radius: Theme.radius
            color: Theme.chrome
            border.color: Theme.line

            ColumnLayout {
                id: chat
                anchors.fill: parent
                anchors.margins: 16
                spacing: 10

                Label {
                    text: qsTr("Room messages").toUpperCase()
                    color: Theme.muted
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    font.letterSpacing: 1.2
                }
                Repeater {
                    model: roomChat
                    Label {
                        required property string from
                        required property string message
                        textFormat: Text.StyledText
                        text: "<b><font color=\"" + (from === root.client.callsign ? Theme.accentText : Theme.okText) + "\">"
                              + from.replace(/</g, "&lt;") + "</font></b> " + message.replace(/</g, "&lt;")
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
                Label {
                    visible: roomChat.count === 0
                    text: qsTr("No message yet.")
                    color: Theme.muted
                }
                RowLayout {
                    Layout.fillWidth: true
                    TextField {
                        id: roomMessage
                        placeholderText: qsTr("Message to the room")
                        Layout.fillWidth: true
                        onAccepted: roomSend.clicked()
                    }
                    Button {
                        id: roomSend
                        icon.source: "../icons/arrow.svg"
                        Material.background: Theme.accent
                        Material.foreground: Theme.accentInk
                        Accessible.name: qsTr("Send")
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Send")
                        enabled: roomMessage.text.trim().length > 0
                        onClicked: { root.client.sendChat(roomMessage.text); roomMessage.clear() }
                        Layout.preferredWidth: 56
                    }
                }
            }
        }
    }
}
