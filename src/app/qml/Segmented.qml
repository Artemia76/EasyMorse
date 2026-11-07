// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EasyMorse

// Row of exclusive choices, the selected one raised
Rectangle {
    id: root

    property var model: []
    property int currentIndex: 0
    // Read by screen readers before each choice
    property string accessibleName

    signal activated(int index)

    color: Theme.surface
    radius: 10
    implicitWidth: row.implicitWidth + 6
    implicitHeight: 38

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: 0
        Repeater {
            model: root.model
            AbstractButton {
                id: choice
                required property string modelData
                required property int index
                readonly property bool selected: root.currentIndex === index
                text: modelData
                implicitHeight: 32
                implicitWidth: choiceLabel.implicitWidth + 24
                checkable: true
                checked: selected
                onClicked: root.activated(index)
                Accessible.name: root.accessibleName.length > 0 ? root.accessibleName + " : " + modelData : modelData
                background: Rectangle {
                    radius: 8
                    color: choice.selected ? Theme.lineStrong : choice.hovered ? Theme.raised : "transparent"
                }
                contentItem: Label {
                    id: choiceLabel
                    text: choice.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 13
                    font.weight: choice.selected ? Font.DemiBold : Font.Medium
                    color: choice.selected ? Theme.text : Theme.textSoft
                }
            }
        }
    }
}
