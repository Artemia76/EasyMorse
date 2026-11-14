// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import EasyMorse

// Copy practice (learning by ear, Koch method): 5 groups of 5 characters
// are played at the speed of the sound and speed controls, the operator
// types what they hear as it comes, with the keyboard or the on-screen
// keypad. The text sent and the mistakes show at the end; 90 % right brings
// a new character.
Page {
    id: root

    readonly property bool compact: height < 520
    readonly property bool narrow: width < 520
    readonly property bool copying: Listening.state === Listening.Playing || Listening.state === Listening.Finishing
    readonly property bool done: Listening.state === Listening.Done
    property string newCharacter

    background: null
    focus: true

    // The series stops when the page is left
    Binding { target: Listening; property: "active"; value: root.visible }
    onVisibleChanged: if (visible) root.forceActiveFocus()

    Settings {
        id: options
        category: "Listening"
        // The keypad is shown by default where there is no keyboard
        property bool keypad: Theme.mobile
    }

    Connections {
        target: Listening
        function onLevelUp(character) {
            root.newCharacter = character
            newCharacterTimer.restart()
        }
        function onStateChanged() {
            if (Listening.state === Listening.Playing)
                root.newCharacter = ""
            root.forceActiveFocus()
        }
    }
    Timer {
        id: newCharacterTimer
        interval: 15000
        onTriggered: root.newCharacter = ""
    }

    function startOrFinish() {
        if (root.copying)
            Listening.finish()
        else
            Listening.start()
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            root.startOrFinish()
            event.accepted = true
        } else if (event.key === Qt.Key_Backspace) {
            Listening.backspace()
            event.accepted = true
        } else if (root.copying && event.text.length === 1 && event.text !== " ") {
            Listening.type(event.text)
            event.accepted = true
        }
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
                    focusPolicy: Qt.NoFocus
                    enabled: !root.copying && Listening.kochLevel > 2
                    onClicked: Listening.kochLevel = Listening.kochLevel - 1
                    Accessible.name: qsTr("One character less")
                    ToolTip.visible: hovered
                    ToolTip.text: Accessible.name
                }
                Label {
                    // Characters learnt, the newest one highlighted
                    textFormat: Text.StyledText
                    text: Listening.kochCharacters.slice(0, -1)
                          + "<font color=\"" + Theme.accentText + "\"><b>" + Listening.kochCharacters.slice(-1) + "</b></font>"
                    font.family: Theme.monoFont
                    font.pixelSize: 15
                    elide: Text.ElideLeft
                    Layout.fillWidth: true
                    Layout.maximumWidth: implicitWidth
                    Layout.minimumWidth: 40
                    Accessible.name: qsTr("%n character(s) learnt", "", Listening.kochLevel)
                }
                RoundButton {
                    text: "+"
                    flat: true
                    focusPolicy: Qt.NoFocus
                    enabled: !root.copying && Listening.kochLevel < Listening.kochMaxLevel
                    onClicked: Listening.kochLevel = Listening.kochLevel + 1
                    Accessible.name: qsTr("One character more")
                    ToolTip.visible: hovered
                    ToolTip.text: Accessible.name
                }
            }
            RowLayout {
                spacing: 8
                Layout.alignment: root.narrow ? Qt.AlignHCenter : Qt.AlignRight

                Button {
                    visible: root.done
                    text: qsTr("Listen again")
                    icon.source: "../icons/play.svg"
                    icon.width: 18
                    icon.height: 18
                    flat: true
                    focusPolicy: Qt.NoFocus
                    enabled: Trainer.audioRunning && !Trainer.playing
                    onClicked: Listening.replay()
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Play the series again, with its text under your eyes")
                }
                Button {
                    visible: root.copying
                    text: qsTr("Stop")
                    flat: true
                    focusPolicy: Qt.NoFocus
                    onClicked: Listening.stop()
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Give up this series")
                }
                Button {
                    text: root.copying ? qsTr("Finish") : root.done ? qsTr("New series") : qsTr("Start")
                    highlighted: true
                    focusPolicy: Qt.NoFocus
                    enabled: root.copying || Trainer.audioRunning
                    onClicked: root.startOrFinish()
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Enter key")
                }
            }
        }

        // ---- New character
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

        // ---- Copy, then correction
        Card {
            title: root.copying ? qsTr("Your copy")
                                : root.done ? qsTr("Correction") : qsTr("Copy by ear")
            Layout.fillWidth: true

            ColumnLayout {
                width: parent.width
                spacing: root.compact ? 8 : 12

                Label {
                    // Phones: the keypad needs the room while copying
                    visible: !root.done && !(root.copying && (root.narrow || root.compact))
                    text: Listening.state === Listening.Finishing
                          ? qsTr("The sound is over: type the last characters, the series ends a few seconds after the last one.")
                          : root.copying ? qsTr("Type each character as soon as you hear it. A character you miss: leave it and go on.")
                                         : qsTr("%1 groups of %2 characters are played. Type what you hear as it comes; 90 % right brings a new character.")
                                           .arg(Listening.groupCount).arg(Listening.groupLength)
                    color: Theme.muted
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                // One box per character, in groups
                Flow {
                    id: groups
                    // Two groups per line on phones
                    readonly property int cell: root.compact || root.narrow ? 28 : 38
                    spacing: root.compact || root.narrow ? 10 : 16
                    Layout.fillWidth: true

                    Repeater {
                        model: Listening.groupCount

                        Row {
                            id: group
                            required property int index
                            spacing: 3

                            Repeater {
                                model: Listening.groupLength

                                Column {
                                    id: cell
                                    required property int index
                                    readonly property int position: group.index * Listening.groupLength + index
                                    readonly property var result: root.done && position < Listening.results.length
                                                                  ? Listening.results[position] : null
                                    readonly property bool current: root.copying && position === Listening.typed.length
                                    spacing: 1

                                    Rectangle {
                                        width: groups.cell
                                        height: groups.cell + 6
                                        radius: 6
                                        color: cell.result === null ? Theme.raised
                                               : cell.result.status === Listening.Correct ? Theme.okBackground : "transparent"
                                        border.width: cell.current || (cell.result !== null && cell.result.status !== Listening.Correct) ? 2 : 1
                                        border.color: cell.current ? Theme.accent
                                                      : cell.result === null ? Theme.line
                                                      : cell.result.status === Listening.Correct ? Theme.ok : Theme.error

                                        Label {
                                            anchors.centerIn: parent
                                            text: cell.result !== null ? (cell.result.copied.length > 0 ? cell.result.copied : "·")
                                                                       : Listening.typed.charAt(cell.position)
                                            font.family: Theme.monoFont
                                            font.pixelSize: groups.cell < 30 ? 18 : 22
                                            font.weight: Font.DemiBold
                                            color: cell.result === null ? Theme.text
                                                   : cell.result.status === Listening.Correct ? Theme.okText : Theme.error
                                        }
                                    }
                                    Label {
                                        // Character sent, under each mistake
                                        visible: root.done
                                        width: groups.cell
                                        horizontalAlignment: Text.AlignHCenter
                                        text: cell.result !== null && cell.result.status !== Listening.Correct
                                              ? cell.result.expected : " "
                                        font.family: Theme.monoFont
                                        font.pixelSize: 15
                                        font.weight: Font.Bold
                                        color: Theme.accentText
                                    }
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    visible: root.done
                    spacing: 12
                    Layout.fillWidth: true
                    Label {
                        text: Listening.passed ? qsTr("Passed") : qsTr("Try again")
                        color: Listening.passed ? Theme.okText : Theme.error
                        font.pixelSize: 18
                        font.weight: Font.Bold
                    }
                    Label {
                        text: qsTr("%1/%2 · %3 %").arg(Listening.correct).arg(Listening.total).arg(Listening.percent)
                        font.family: Theme.monoFont
                        color: Theme.textSoft
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        visible: Listening.extra > 0
                        text: qsTr("%n extra character(s)", "", Listening.extra)
                        color: Theme.muted
                    }
                }
                Label {
                    visible: root.done
                    text: Listening.passed ? qsTr("90 % needed. Well done: the course goes on.")
                                           : qsTr("90 % needed. The characters in yellow are the ones sent: listen again to the series to fix them.")
                    color: Theme.muted
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }

        // ---- Keypad
        Switch {
            text: qsTr("On-screen keypad")
            checked: options.keypad
            focusPolicy: Qt.NoFocus
            onToggled: options.keypad = checked
        }
        CopyPad {
            // Of no use on the correction, which needs the room
            visible: options.keypad && !root.done
            enabled: root.copying
            characters: Listening.kochCharacters
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 80
            onTyped: (character) => Listening.type(character)
            onErased: Listening.backspace()
        }
        Item {
            visible: !options.keypad || root.done
            Layout.fillHeight: true
        }
    }
}
