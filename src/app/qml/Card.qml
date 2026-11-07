// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import EasyMorse

// Rounded surface with a small capitals title, used like a GroupBox
GroupBox {
    id: control

    padding: 16
    topPadding: title.length > 0 ? heading.implicitHeight + 26 : 16

    label: Label {
        id: heading
        x: control.leftPadding
        y: 14
        width: control.availableWidth
        text: control.title.toUpperCase()
        color: Theme.muted
        elide: Text.ElideRight
        font.pixelSize: 12
        font.weight: Font.DemiBold
        font.letterSpacing: 1.2
    }

    background: Rectangle {
        radius: Theme.radius
        color: Theme.surface
    }
}
