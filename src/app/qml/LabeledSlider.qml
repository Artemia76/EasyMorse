// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EasyMorse

// Slider with its name and value above it, bound to a setting
ColumnLayout {
    id: root

    // Name of the setting
    property string text
    // Value as shown, %1 replaced by the displayed value
    property string format: "%1"
    property real value
    property alias from: slider.from
    property alias to: slider.to
    property alias stepSize: slider.stepSize
    // Value shown, e.g. a percentage
    property var display: (v) => Math.round(v)

    // Emitted when the user moves the slider
    signal moved(real value)

    spacing: 0
    Layout.fillWidth: true

    RowLayout {
        Layout.fillWidth: true
        Label {
            text: root.text
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
        Label {
            text: root.format.arg(root.display(slider.value))
            font.family: Theme.monoFont
            color: root.enabled ? Theme.accentText : Theme.muted
        }
    }
    Slider {
        id: slider
        snapMode: Slider.SnapAlways
        Layout.fillWidth: true
        onMoved: root.moved(value)
        Accessible.name: root.text
    }
    // Follow the setting, also when it is changed elsewhere
    Binding {
        target: slider
        property: "value"
        value: root.value
        when: !slider.pressed
    }
}
