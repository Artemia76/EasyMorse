// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EasyMorse

// Straight key or iambic paddles; what is keyed is decoded at the bottom of the window
Page {
    id: root

    readonly property bool iambic: MorseSettings.keyerMode === 1

    background: null

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Segmented {
                model: [qsTr("Straight key"), qsTr("Iambic")]
                currentIndex: MorseSettings.keyerMode
                accessibleName: qsTr("Key")
                onActivated: (index) => MorseSettings.keyerMode = index
            }
            Label {
                visible: root.iambic
                text: qsTr("Mode %1 · %2 WPM").arg(MorseSettings.iambicMode === 0 ? "A" : "B").arg(MorseSettings.charSpeed)
                color: Theme.muted
                font.family: Theme.monoFont
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Item { visible: !root.iambic; Layout.fillWidth: true }
            Rectangle {
                // Lit while the key sends
                width: 14; height: 14; radius: 7
                color: Trainer.keyDown ? Theme.accent : Theme.lineStrong
                Accessible.ignored: true
            }
        }

        Label {
            // Dots and dashes of the character being keyed
            text: Theme.pattern(Trainer.decoderPattern).split("").join(" ") || " "
            font.family: Theme.monoFont
            font.pixelSize: 44
            color: Theme.accentText
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
            Layout.preferredHeight: 60
        }
        KeyPad {
            visible: !root.iambic
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 160
            Layout.maximumHeight: 340
            Layout.alignment: Qt.AlignHCenter
        }
        PaddlePad {
            visible: root.iambic
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 160
            Layout.maximumHeight: 300
        }
        Label {
            text: Theme.mobile ? (root.iambic ? qsTr("One finger on each paddle; hold both to alternate.")
                                              : qsTr("Press the key as long as the sign lasts."))
                  : root.iambic ? qsTr("%1: dots · %2: dashes · serial paddles")
                                .arg(Keyboard.keyName(MorseSettings.dotKey)).arg(Keyboard.keyName(MorseSettings.dashKey))
                              : qsTr("Space bar (key focused), %1 or serial key").arg(Keyboard.keyName(MorseSettings.straightKey))
            color: Theme.muted
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }
        Item { Layout.fillHeight: true; Layout.maximumHeight: 40 }
    }
}
