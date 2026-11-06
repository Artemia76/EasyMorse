// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
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

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width, parent.height)
        height: width
        radius: width / 2
        // Lit by any key: this pad, Alt, serial key
        color: Trainer.keyDown ? Material.accent : Material.color(Material.Grey, Material.Shade700)
        border.width: root.activeFocus ? 4 : 0
        border.color: Material.primary
        scale: Trainer.keyDown ? 0.96 : 1.0

        Behavior on scale { NumberAnimation { duration: 40 } }

        Label {
            anchors.centerIn: parent
            width: parent.width * 0.7
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: Trainer.playing ? qsTr("Listening…") : qsTr("Press and hold")
            color: "white"
            font.pixelSize: 18
        }
    }

    PointHandler {
        id: pointer
        enabled: !Trainer.playing
        onActiveChanged: if (active) root.forceActiveFocus()
    }
}
