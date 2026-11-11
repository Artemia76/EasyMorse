// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import EasyMorse

// Big round key operated by mouse, finger or the space bar when focused
Item {
    id: root

    implicitWidth: 220
    implicitHeight: 220
    activeFocusOnTab: true

    readonly property bool pressed: pointer.active || spaceDown
    property bool spaceDown: false

    onPressedChanged: pressed ? Trainer.manualKeyer.press() : Trainer.manualKeyer.release()
    // Do not leave the key down when the page is hidden
    onVisibleChanged: if (!visible) spaceDown = false

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Space && !event.isAutoRepeat) {
            spaceDown = true
            event.accepted = true
        }
    }
    Keys.onReleased: (event) => {
        if (event.key === Qt.Key_Space && !event.isAutoRepeat) {
            spaceDown = false
            event.accepted = true
        }
    }

    Item {
        id: dial
        anchors.horizontalCenter: parent.horizontalCenter
        // Centred in the room left above the capture button
        y: (parent.height - capture.room - height) / 2
        width: Math.min(parent.width, parent.height - capture.room) / 1.12
        height: width

        // Glow, stronger while any key is down: this pad, Alt, serial key
        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 1.12
            height: width
            radius: width / 2
            color: Qt.alpha(Theme.accent, Trainer.keyDown ? 0.26 : 0.08)
            Behavior on color { ColorAnimation { duration: 60 } }
        }
        // Base
        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: Theme.surface
            border.width: 2
            border.color: Theme.lineStrong
        }
        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.88
            height: width
            radius: width / 2
            color: Theme.raised
            border.width: root.activeFocus ? 3 : 0
            border.color: Theme.accent
        }
        // The key itself
        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.62
            height: width
            radius: width / 2
            scale: Trainer.keyDown ? 0.95 : 1.0
            opacity: Trainer.playing ? 0.5 : 1.0
            gradient: Gradient {
                GradientStop { position: 0.0; color: Trainer.keyDown ? "#FFE3A8" : Theme.accentLight }
                GradientStop { position: 0.55; color: Theme.accent }
                GradientStop { position: 1.0; color: Theme.accentDeep }
            }
            Behavior on scale { NumberAnimation { duration: 40 } }

            Label {
                anchors.centerIn: parent
                width: parent.width * 0.8
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: Trainer.playing ? qsTr("Listening…") : qsTr("Press")
                color: Theme.accentInk
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }
        }
    }

    MouseCaptureButton {
        id: capture
        // Height kept free under the key
        readonly property real room: visible ? height + 4 : 0
        target: dial
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
    }

    PointHandler {
        id: pointer
        // On the dial only: not on the capture button
        parent: dial
        enabled: !Trainer.playing
        onActiveChanged: if (active) root.forceActiveFocus()
    }
}
