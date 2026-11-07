// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import EasyMorse

// Entry of the navigation: icon and label, side by side on the desktop
// rail, stacked in the bottom bar of phones
AbstractButton {
    id: control

    property bool vertical: false
    // Icon only, on narrow desktop windows
    property bool compact: false

    checkable: true
    implicitHeight: vertical ? 56 : 44
    implicitWidth: vertical ? 72 : compact ? 44 : 172
    ToolTip.visible: compact && hovered
    ToolTip.text: text
    focusPolicy: Qt.TabFocus
    Accessible.name: text

    background: Rectangle {
        visible: !control.vertical
        radius: 10
        color: control.checked ? Theme.accent
                               : control.hovered || control.visualFocus ? Theme.raised : "transparent"
    }

    contentItem: GridLayout {
        flow: control.vertical ? GridLayout.TopToBottom : GridLayout.LeftToRight
        columnSpacing: 12
        rowSpacing: 2

        Rectangle {
            // Pill behind the icon of the page shown, on phones
            Layout.alignment: control.vertical ? Qt.AlignHCenter : Qt.AlignVCenter
            Layout.leftMargin: control.vertical ? 0 : 12
            implicitWidth: control.vertical ? 52 : 20
            implicitHeight: control.vertical ? 28 : 20
            radius: 14
            color: control.vertical && control.checked ? Qt.alpha(Theme.accent, 0.18) : "transparent"
            IconImage {
                anchors.centerIn: parent
                source: control.icon.source
                sourceSize: Qt.size(20, 20)
                color: !control.vertical && control.checked ? Theme.accentInk
                       : control.checked ? Theme.accentText : Theme.textSoft
            }
        }
        Label {
            visible: !control.compact
            text: control.text
            Layout.fillWidth: !control.vertical
            Layout.alignment: control.vertical ? Qt.AlignHCenter : Qt.AlignVCenter
            elide: Text.ElideRight
            font.pixelSize: control.vertical ? 12 : 14
            font.weight: control.checked ? Font.DemiBold : Font.Normal
            color: !control.vertical && control.checked ? Theme.accentInk
                   : control.checked ? Theme.accentText : Theme.textSoft
        }
    }
}
