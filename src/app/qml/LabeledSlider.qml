// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Slider with a caption showing its value, bound to a setting
ColumnLayout {
    id: root

    // Caption with %1 replaced by the displayed value
    property string text
    property real value
    property alias from: slider.from
    property alias to: slider.to
    property alias stepSize: slider.stepSize
    // Value shown in the caption, e.g. a percentage
    property var display: (v) => Math.round(v)

    // Emitted when the user moves the slider
    signal moved(real value)

    spacing: 0
    Layout.fillWidth: true

    Label {
        text: root.text.arg(root.display(slider.value))
    }
    Slider {
        id: slider
        snapMode: Slider.SnapAlways
        Layout.fillWidth: true
        onMoved: root.moved(value)
    }
    // Follow the setting, also when it is changed elsewhere
    Binding {
        target: slider
        property: "value"
        value: root.value
        when: !slider.pressed
    }
}
