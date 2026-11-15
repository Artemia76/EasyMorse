// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import EasyMorse

// Takes the mouse for a mouse wired as a Morse key: its buttons then key
// wherever the pointer is, and the pointer is held still over the target
Button {
    id: control

    // Item the pointer is held over
    required property Item target
    // Icon only, the text in the tooltip
    property bool compact: false

    visible: !Theme.mobile
    text: qsTr("Capture the mouse")
    icon.source: "../icons/mouse.svg"
    icon.width: 18
    icon.height: 18
    display: compact ? AbstractButton.IconOnly : AbstractButton.TextBesideIcon
    Accessible.name: text
    flat: true
    enabled: !Mouse.captured
    onClicked: Mouse.capture(control.target)
    // Hover is not updated while the mouse is captured
    ToolTip.visible: hovered && !Mouse.captured
    ToolTip.delay: 600
    ToolTip.text: (compact ? text + ". " : "") + qsTr("For a mouse wired as a key: left button = key or dots, right button = dashes. %1 frees the mouse.")
                  .arg(Keyboard.keyName(MorseSettings.releaseKey))
}
