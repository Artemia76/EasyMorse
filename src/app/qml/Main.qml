// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Effects
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
    // iOS: drawn up to the edges of the screen (undefined before Qt 6.9, the
    // window then stays inside the safe area and the margins below are 0)
    flags: Qt.platform.os === "ios" ? Qt.Window | Qt.ExpandedClientAreaHint : Qt.Window
    color: Theme.ground
    font.family: Theme.uiFont
    font.pixelSize: 14

    readonly property bool darkTheme: UiSettings.theme === UiSettings.Dark
        || (UiSettings.theme === UiSettings.System && Qt.styleHints.colorScheme !== Qt.ColorScheme.Light)
    Binding { target: Theme; property: "dark"; value: window.darkTheme }

    Material.theme: darkTheme ? Material.Dark : Material.Light
    Material.background: Theme.ground
    Material.foreground: Theme.text
    Material.accent: Theme.accentText
    Material.primary: Theme.accentDeep
    Material.roundedScale: Material.SmallScale

    // Desktop: navigation rail on the left, sound and speed on the right.
    // Phone: navigation at the bottom, sound and speed on a page of their own
    readonly property bool wide: width >= 800
    readonly property bool roomy: width >= 1040
    // Low windows (phones held sideways): tighter header, strip and navigation
    readonly property bool compact: height < 560
    // Room taken by the notch and the home indicator: ApplicationWindow keeps
    // its content clear of it, the header and the navigation bar extend into it
    Loader {
        id: safeArea
        active: Qt.platform.os === "ios" && Qt.ExpandedClientAreaHint !== undefined
        source: "SafeAreaProbe.qml"
    }
    readonly property int safeTop: safeArea.item ? safeArea.item.marginTop : 0
    readonly property int safeBottom: safeArea.item ? safeArea.item.marginBottom : 0
    readonly property int safeLeft: safeArea.item ? safeArea.item.marginLeft : 0
    readonly property int safeRight: safeArea.item ? safeArea.item.marginRight : 0
    readonly property int pageCount: 6
    readonly property int controlsPage: 6
    property int currentPage: 0
    // Pages where Morse is heard show the sound and speed controls beside them
    readonly property bool sidePanel: wide && (currentPage === 0 || currentPage === 1 || currentPage === 3)

    // Window size is kept between sessions
    Settings {
        category: "MainWindow"
        property alias width: window.width
        property alias height: window.height
    }
    onWideChanged: if (wide && currentPage >= pageCount) currentPage = 0
    // Android back button: first back to the first page, then quit
    onClosing: (close) => {
        if (Qt.platform.os === "android" && currentPage !== 0) {
            close.accepted = false
            currentPage = 0
        }
    }

    readonly property var pages: [
        { title: qsTr("Send"), icon: "../icons/send.svg" },
        { title: qsTr("Key"), icon: "../icons/key.svg" },
        { title: qsTr("Network"), icon: "../icons/network.svg" },
        { title: qsTr("Glossary"), icon: "../icons/glossary.svg" },
        { title: qsTr("Log"), icon: "../icons/log.svg" },
        { title: qsTr("Options"), icon: "../icons/options.svg" },
        { title: qsTr("Sound and speed"), icon: "../icons/sliders.svg" }
    ]

    readonly property var client: Trainer.lobby.client
    readonly property bool inRoom: client.state === SignalingClient.Online && client.room.length > 0

    header: Rectangle {
        implicitHeight: (window.compact ? 44 : 56) + window.safeTop
        color: Theme.chrome

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.line
        }

        RowLayout {
            anchors.fill: parent
            anchors.topMargin: window.safeTop
            anchors.leftMargin: 16 + window.safeLeft
            anchors.rightMargin: 16 + window.safeRight
            spacing: 12

            Item {
                Layout.preferredWidth: window.compact ? 28 : 34
                Layout.preferredHeight: window.compact ? 28 : 34
                Image {
                    // The walrus ("morse" in French) of the original icon
                    id: logo
                    anchors.fill: parent
                    source: "qrc:/gfx/Morse_128x128.png"
                    sourceSize: Qt.size(72, 72)
                    visible: false
                }
                Rectangle {
                    id: logoMask
                    anchors.fill: parent
                    radius: 8
                    visible: false
                    layer.enabled: true
                }
                MultiEffect {
                    anchors.fill: parent
                    source: logo
                    maskEnabled: true
                    maskSource: logoMask
                }
            }
            Label {
                text: "EasyMorse"
                font.pixelSize: 18
                font.weight: Font.Bold
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            Chip {
                visible: !Trainer.audioRunning
                text: qsTr("No sound output")
                foreground: Theme.error
                light: Theme.error
            }
            Chip {
                visible: window.inRoom
                text: window.wide ? qsTr("Room %1 · %n station(s)", "", Trainer.lobby.connectedCount + 1).arg(window.client.room)
                                  : window.client.room + " · " + (Trainer.lobby.connectedCount + 1)
                color: Theme.okBackground
                foreground: Theme.okText
                light: Theme.ok
            }
            Chip {
                visible: window.width >= 520
                textFormat: Text.StyledText
                text: qsTr("Decoder <b>%1</b> WPM").arg(Math.round(Trainer.decoderSpeed))
                      + (MorseSettings.decoderAutoAdapt ? "" : " · " + qsTr("locked"))
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        // The navigation bar of phones goes down to the edge of the screen
        anchors.bottomMargin: window.wide ? 0 : -window.safeBottom
        spacing: 0

        RowLayout {
            spacing: 0
            Layout.fillWidth: true
            Layout.fillHeight: true

            // ---- Navigation rail (desktop)
            Rectangle {
                visible: window.wide
                color: Theme.chrome
                Layout.fillHeight: true
                Layout.preferredWidth: window.roomy ? 196 : 68

                // Scrolls when the window is too low for all the pages
                ListView {
                    anchors.fill: parent
                    anchors.margins: window.compact ? 8 : 12
                    spacing: window.compact ? 2 : 4
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    model: 6
                    delegate: NavButton {
                        required property int index
                        width: ListView.view.width
                        height: window.compact ? 38 : 44
                        text: window.pages[index].title
                        icon.source: window.pages[index].icon
                        compact: !window.roomy
                        checked: window.currentPage === index
                        onClicked: window.currentPage = index
                    }
                }
                Rectangle {
                    anchors.right: parent.right
                    width: 1
                    height: parent.height
                    color: Theme.line
                }
            }

            StackLayout {
                currentIndex: window.currentPage
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

            // ---- Sound and speed (desktop)
            Rectangle {
                visible: window.sidePanel
                color: Theme.chrome
                Layout.fillHeight: true
                Layout.preferredWidth: 300

                Rectangle {
                    width: 1
                    height: parent.height
                    color: Theme.line
                }
                ControlsPanel {
                    anchors.fill: parent
                    anchors.leftMargin: 1
                }
            }
        }

        // Decoded text, whatever the page shown
        DecoderPanel {
            compact: window.compact
            Layout.fillWidth: true
        }

        // ---- Navigation bar (phone)
        Rectangle {
            visible: !window.wide
            color: Theme.chrome
            Layout.fillWidth: true
            implicitHeight: 64 + window.safeBottom

            Rectangle {
                width: parent.width
                height: 1
                color: Theme.line
            }
            RowLayout {
                anchors.fill: parent
                anchors.topMargin: 4
                anchors.bottomMargin: window.safeBottom
                spacing: 0

                Repeater {
                    model: 3
                    NavButton {
                        required property int index
                        vertical: true
                        text: window.pages[index].title
                        icon.source: window.pages[index].icon
                        checked: window.currentPage === index
                        onClicked: window.currentPage = index
                        Layout.fillWidth: true
                    }
                }
                NavButton {
                    id: moreButton
                    vertical: true
                    text: qsTr("More")
                    icon.source: "../icons/more.svg"
                    checked: window.currentPage >= 3
                    onClicked: { checked = Qt.binding(() => window.currentPage >= 3); moreMenu.open() }
                    Layout.fillWidth: true

                    Menu {
                        id: moreMenu
                        y: -height
                        Repeater {
                            model: [3, 4, 5, 6]
                            MenuItem {
                                required property int modelData
                                text: window.pages[modelData].title
                                icon.source: window.pages[modelData].icon
                                onTriggered: window.currentPage = modelData
                            }
                        }
                    }
                }
            }
        }
    }
}
