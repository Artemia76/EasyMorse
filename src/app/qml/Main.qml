// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import EasyMorse

ApplicationWindow {
    id: window

    width: 1100
    height: 720
    minimumWidth: 360
    minimumHeight: 480
    visible: true
    title: "EasyMorse " + Qt.application.version

    Material.theme: UiSettings.theme === UiSettings.Dark ? Material.Dark
                  : UiSettings.theme === UiSettings.Light ? Material.Light : Material.System
    Material.primary: Material.Indigo
    Material.accent: Material.Amber

    // On narrow screens the sound and speed controls get their own tab
    readonly property bool wide: width >= 800
    readonly property int pageCount: 6
    property int currentPage: 0

    onCurrentPageChanged: tabs.currentIndex = currentPage

    // Window size is kept between sessions
    Settings {
        category: "MainWindow"
        property alias width: window.width
        property alias height: window.height
    }
    onWideChanged: if (wide && currentPage >= pageCount) currentPage = 0

    header: ToolBar {
        // Same dark bar in light and dark themes, white text
        Material.background: Material.color(Material.Indigo, Material.Shade700)
        Material.foreground: "white"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 12

            Label {
                text: "EasyMorse"
                font.pixelSize: 20
                font.bold: true
                Layout.fillWidth: true
            }
            Label {
                visible: !Trainer.audioRunning
                text: qsTr("No sound output")
                color: Material.color(Material.Red, Material.Shade200)
            }
            Label {
                text: qsTr("Decoder %1 WPM").arg(Math.round(Trainer.decoderSpeed))
                      + (MorseSettings.decoderAutoAdapt ? "" : " 🔒")
            }
        }
    }

    // Incoming call from a room member, whatever the page shown
    Dialog {
        id: incomingCall
        property string caller
        anchors.centerIn: parent
        modal: true
        title: qsTr("Incoming call")
        Label { text: qsTr("%1 is calling you for a Morse session.").arg(incomingCall.caller) }
        footer: DialogButtonBox {
            Button { text: qsTr("Accept"); DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole }
            Button { text: qsTr("Decline"); DialogButtonBox.buttonRole: DialogButtonBox.RejectRole }
        }
        onAccepted: { Trainer.lobby.accept(); window.currentPage = 2 }
        onRejected: Trainer.lobby.decline()
    }
    Connections {
        target: Trainer.lobby
        function onIncomingCall(pFrom) { incomingCall.caller = pFrom; incomingCall.open() }
        function onCallStateChanged() {
            // The caller gave up meanwhile
            if (Trainer.lobby.callState !== Lobby.Ringing && incomingCall.opened)
                incomingCall.close()
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        ControlsPanel {
            visible: window.wide
            Layout.preferredWidth: 320
            Layout.fillHeight: true
        }

        ToolSeparator {
            visible: window.wide
            orientation: Qt.Vertical
            Layout.fillHeight: true
        }

        ColumnLayout {
            spacing: 0
            Layout.fillWidth: true
            Layout.fillHeight: true

            TabBar {
                id: tabs
                Layout.fillWidth: true
                onCurrentIndexChanged: window.currentPage = currentIndex

                TabButton { text: qsTr("Send"); width: implicitWidth }
                TabButton { text: qsTr("Key"); width: implicitWidth }
                TabButton { text: qsTr("Network"); width: implicitWidth }
                TabButton { text: qsTr("Glossary"); width: implicitWidth }
                TabButton { text: qsTr("Log"); width: implicitWidth }
                TabButton { text: qsTr("Options"); width: implicitWidth }
                TabButton {
                    text: qsTr("Settings")
                    width: window.wide ? 0 : implicitWidth
                    visible: !window.wide
                }
            }

            StackLayout {
                currentIndex: tabs.currentIndex
                Layout.fillWidth: true
                Layout.fillHeight: true

                SendPage {}
                KeyPage {}
                NetworkPage {}
                GlossaryPage {
                    onPlayRequested: (text) => {
                        Trainer.play(text)
                    }
                }
                LogPage {}
                OptionsPage {}
                ControlsPanel {}
            }
        }
    }
}
