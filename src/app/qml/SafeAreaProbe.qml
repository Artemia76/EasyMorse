// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick

// Room taken by the notch and the home indicator. A file of its own, loaded by
// Main.qml only when Qt has the SafeArea type (6.9 or newer)
Item {
    readonly property int marginTop: Window.window ? Window.window.SafeArea.margins.top : 0
    readonly property int marginBottom: Window.window ? Window.window.SafeArea.margins.bottom : 0
    readonly property int marginLeft: Window.window ? Window.window.SafeArea.margins.left : 0
    readonly property int marginRight: Window.window ? Window.window.SafeArea.margins.right : 0
}
