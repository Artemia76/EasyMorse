// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import EasyMorse

// Keying practice: a group of Koch characters to key, then the timeline of
// the ideal and real keying and an appreciation. What is keyed is decoded at
// the bottom of the window as on the other pages.
Page {
    id: root

    readonly property bool iambic: MorseSettings.keyerMode === 1
    // Low window (phone held sideways): no pattern under the letters, smaller key
    readonly property bool compact: height < 520
    readonly property bool narrow: width < 520
    // New character of the course, shown for a while
    property string newCharacter

    background: null

    // The practice listens to the key only while this page is shown
    Binding { target: Practice; property: "active"; value: root.visible }

    Connections {
        target: Practice
        function onLevelUp(character) {
            root.newCharacter = character
            newCharacterTimer.restart()
        }
    }
    Timer {
        id: newCharacterTimer
        interval: 8000
        onTriggered: root.newCharacter = ""
    }

    function verdictText() {
        switch (Practice.verdict) {
        case Practice.Excellent: return qsTr("Excellent")
        case Practice.Good: return qsTr("Good")
        case Practice.NeedsWork: return qsTr("Keep practising")
        }
        return ""
    }
    function verdictColor() {
        switch (Practice.verdict) {
        case Practice.Excellent: return Theme.okText
        case Practice.Good: return Theme.accentText
        }
        return Theme.error
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.compact ? 12 : 20
        spacing: root.compact ? 8 : 14

        // ---- Course and actions, on two lines on phones
        GridLayout {
            columns: root.narrow ? 1 : 2
            rowSpacing: 0
            columnSpacing: 8
            Layout.fillWidth: true

            RowLayout {
                spacing: 8
                Layout.fillWidth: true

                Label {
                    text: qsTr("Koch")
                    color: Theme.muted
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    font.letterSpacing: 1.2
                }
                RoundButton {
                    text: "−"
                    flat: true
                    enabled: Practice.kochLevel > 2
                    onClicked: Practice.kochLevel = Practice.kochLevel - 1
                    Accessible.name: qsTr("One character less")
                    ToolTip.visible: hovered
                    ToolTip.text: Accessible.name
                }
                Label {
                    // Characters learnt, the newest one highlighted
                    textFormat: Text.StyledText
                    text: Practice.kochCharacters.slice(0, -1)
                          + "<font color=\"" + Theme.accentText + "\"><b>" + Practice.kochCharacters.slice(-1) + "</b></font>"
                    font.family: Theme.monoFont
                    font.pixelSize: 15
                    elide: Text.ElideLeft
                    Layout.fillWidth: true
                    Layout.maximumWidth: implicitWidth
                    Layout.minimumWidth: 40
                    Accessible.name: qsTr("%n character(s) learnt", "", Practice.kochLevel)
                }
                RoundButton {
                    text: "+"
                    flat: true
                    enabled: Practice.kochLevel < Practice.kochMaxLevel
                    onClicked: Practice.kochLevel = Practice.kochLevel + 1
                    Accessible.name: qsTr("One character more")
                    ToolTip.visible: hovered
                    ToolTip.text: Accessible.name
                }
            }
            RowLayout {
                spacing: 8
                Layout.alignment: root.narrow ? Qt.AlignHCenter : Qt.AlignRight

                Button {
                    text: qsTr("Listen")
                    icon.source: "../icons/play.svg"
                    icon.width: 18
                    icon.height: 18
                    flat: true
                    enabled: Trainer.audioRunning && !Trainer.playing
                    onClicked: Practice.listen()
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Play the group as a model")
                }
                MouseCaptureButton {
                    target: root.iambic ? paddlePad : keyPad
                    iconOnly: true
                }
                Button {
                    text: qsTr("New group")
                    flat: true
                    enabled: !Trainer.playing
                    onClicked: Practice.newGroup()
                }
            }
        }

        // ---- Group to key
        Rectangle {
            visible: root.newCharacter.length > 0
            radius: Theme.radius
            color: Theme.okBackground
            Layout.fillWidth: true
            implicitHeight: newLabel.implicitHeight + 16
            Label {
                id: newLabel
                anchors.centerIn: parent
                width: parent.width - 24
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                textFormat: Text.StyledText
                text: qsTr("New character: <b>%1</b> %2").arg(root.newCharacter)
                      .arg(Theme.pattern(Practice.pattern(root.newCharacter)))
                color: Theme.okText
            }
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: root.compact ? 10 : 18

            Repeater {
                model: Practice.group.split("")

                ColumnLayout {
                    id: letter
                    required property string modelData
                    required property int index
                    // Decoded characters of the group being keyed
                    readonly property bool done: Practice.live && Practice.analysisGroup === Practice.group
                                                 && index < Practice.position
                    readonly property bool correct: done && Practice.characters[index] !== undefined
                                                    && Practice.characters[index].correct
                    readonly property bool current: index === Practice.position
                    spacing: 2

                    Label {
                        text: letter.modelData
                        font.family: Theme.monoFont
                        font.pixelSize: root.compact ? 34 : 44
                        font.weight: Font.DemiBold
                        color: !letter.done ? (letter.current ? Theme.accentText : Theme.text)
                               : letter.correct ? Theme.okText : Theme.error
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Rectangle {
                        // Next character to key
                        implicitWidth: 28
                        implicitHeight: 3
                        radius: 1.5
                        color: letter.current ? Theme.accent : "transparent"
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Label {
                        visible: !root.compact
                        text: Theme.pattern(Practice.pattern(letter.modelData))
                        color: Theme.textSoft
                        font.family: Theme.monoFont
                        font.pixelSize: 16
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }
        }

        // ---- Timeline and appreciation
        Card {
            title: Practice.live ? qsTr("Your keying")
                                 : Practice.scored ? qsTr("Last group · %1").arg(Practice.analysisGroup)
                                                   : qsTr("Timeline")
            Layout.fillWidth: true

            ColumnLayout {
                width: parent.width
                spacing: 10

                KeyingTimeline {
                    visible: Practice.realSegments.length > 0
                    Layout.fillWidth: true
                    Layout.preferredHeight: implicitHeight
                }
                Label {
                    visible: Practice.realSegments.length === 0
                    text: root.iambic ? qsTr("Key the group with the paddles: the spacing between the letters is judged.")
                                      : qsTr("Key the group: the ideal keying of each letter appears above yours.")
                    color: Theme.muted
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                RowLayout {
                    visible: Practice.scored && !Practice.live
                    spacing: 12
                    Layout.fillWidth: true
                    Label {
                        text: root.verdictText()
                        color: root.verdictColor()
                        font.pixelSize: 18
                        font.weight: Font.Bold
                    }
                    Label {
                        text: qsTr("%1/100").arg(Practice.score)
                        font.family: Theme.monoFont
                        color: Theme.textSoft
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: qsTr("≈ %1 WPM").arg(Math.round(1200 / Math.max(1, Practice.unitMs)))
                        color: Theme.muted
                        font.family: Theme.monoFont
                    }
                }
                Repeater {
                    model: Practice.scored && !Practice.live ? Practice.advice : []
                    Label {
                        required property string modelData
                        text: "• " + modelData
                        color: Theme.textSoft
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
                Label {
                    // Colors of the timeline
                    visible: Practice.realSegments.length > 0 && !root.compact
                    textFormat: Text.StyledText
                    text: "<font color=\"" + Theme.ok + "\">■</font> " + qsTr("right")
                          + "   <font color=\"" + Theme.accent + "\">■</font> " + qsTr("20 to 40 % off")
                          + "   <font color=\"" + Theme.error + "\">■</font> " + qsTr("more")
                          + "   ┊ " + qsTr("next letter")
                    color: Theme.muted
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }

        // ---- Key
        KeyPad {
            id: keyPad
            visible: !root.iambic
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 90
            Layout.maximumHeight: 240
            Layout.alignment: Qt.AlignHCenter
        }
        PaddlePad {
            id: paddlePad
            visible: root.iambic
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 90
            Layout.maximumHeight: 220
        }
    }
}
