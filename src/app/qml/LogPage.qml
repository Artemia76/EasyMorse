// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import EasyMorse

// Application log
Page {
    id: root

    header: ToolBar {
        Material.background: "transparent"
        Material.elevation: 0

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            spacing: 8

            ComboBox {
                model: [qsTr("Essential"), qsTr("Normal"), qsTr("Verbose")]
                implicitContentWidthPolicy: ComboBox.WidestText
                Layout.minimumWidth: implicitWidth
                currentIndex: UiSettings.logLevel
                onActivated: (index) => UiSettings.logLevel = index
            }
            TextField {
                placeholderText: qsTr("Filter (wildcards)")
                text: UiSettings.logFilter
                onEditingFinished: UiSettings.logFilter = text || "*"
                Layout.fillWidth: true
                Layout.minimumWidth: 60
            }
            ToolButton {
                text: qsTr("Clear")
                onClicked: LogModel.clear()
            }
            ToolButton {
                text: qsTr("Save")
                onClicked: saveDialog.open()
            }
        }
    }

    ListView {
        id: list
        anchors.fill: parent
        anchors.margins: 8
        clip: true
        model: LogModel
        ScrollBar.vertical: ScrollBar {}
        // Follow new messages
        onCountChanged: positionViewAtEnd()

        delegate: Label {
            required property string message
            required property color messageColor

            width: list.width
            text: message
            wrapMode: Text.WrapAnywhere
            font.family: "monospace"
            // Log colors are chosen for a light background
            color: palette.window.hslLightness < 0.5 ? Qt.lighter(messageColor, 1.8) : messageColor
        }
    }

    FileDialog {
        id: saveDialog
        fileMode: FileDialog.SaveFile
        defaultSuffix: "html"
        nameFilters: [qsTr("Web page (*.html *.htm)")]
        onAccepted: {
            if (!LogModel.saveHtml(selectedFile))
                console.warn("Cannot write", selectedFile)
        }
    }

    Binding {
        target: LogModel
        property: "maxLines"
        value: UiSettings.logMaxLines
    }
}
