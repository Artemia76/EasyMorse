// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EasyMorse

// Two touch paddles for the iambic keyer: dots on the left, dashes on the
// right (the other way round when reversed). Both can be held at once.
Item {
    id: root

    implicitWidth: 360
    implicitHeight: 220

    // Do not leave a contact closed when the page is hidden
    onVisibleChanged: if (!visible) Trainer.manualPaddles.release()

    RowLayout {
        id: paddles
        anchors.horizontalCenter: parent.horizontalCenter
        // Centred in the room left above the capture button
        y: (parent.height - capture.room - height) / 2
        width: Math.min(parent.width, 520)
        height: Math.min(parent.height - capture.room, 260)
        spacing: 16

        Repeater {
            // Left paddle first
            model: [false, true]

            Item {
                id: paddle
                required property bool modelData
                readonly property bool rightSide: modelData
                // The touch paddle on this side closes this contact
                readonly property bool dash: rightSide !== MorseSettings.paddlesReversed
                readonly property bool closed: dash ? Trainer.dashPaddle : Trainer.dotPaddle

                Layout.fillWidth: true
                Layout.fillHeight: true

                Rectangle {
                    anchors.fill: parent
                    radius: 24
                    color: Theme.surface
                    border.width: 2
                    border.color: Theme.lineStrong
                }
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 14
                    radius: 16
                    scale: paddle.closed ? 0.97 : 1.0
                    opacity: Trainer.playing ? 0.5 : 1.0
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: paddle.closed ? "#FFE3A8" : Theme.accentLight }
                        GradientStop { position: 0.55; color: Theme.accent }
                        GradientStop { position: 1.0; color: Theme.accentDeep }
                    }
                    Behavior on scale { NumberAnimation { duration: 40 } }

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 4
                        Label {
                            text: paddle.dash ? "−" : "·"
                            font.family: Theme.monoFont
                            font.pixelSize: Math.max(24, Math.min(56, paddle.height * 0.3))
                            font.weight: Font.Bold
                            color: Theme.accentInk
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Label {
                            visible: paddle.height >= 120
                            text: paddle.dash ? qsTr("Dashes") : qsTr("Dots")
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                            color: Theme.accentInk
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                PointHandler {
                    id: touch
                    enabled: !Trainer.playing
                    // Wired like a real paddle: left contact = dots, right = dashes;
                    // the reversal is applied by the Trainer to every paddle
                    onActiveChanged: {
                        if (paddle.rightSide)
                            Trainer.manualPaddles.setDash(active)
                        else
                            Trainer.manualPaddles.setDot(active)
                    }
                }
                Accessible.role: Accessible.Button
                Accessible.name: paddle.dash ? qsTr("Dash paddle") : qsTr("Dot paddle")
            }
        }
    }

    MouseCaptureButton {
        id: capture
        // Height kept free under the paddles
        readonly property real room: visible ? height + 4 : 0
        target: paddles
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
    }
}
