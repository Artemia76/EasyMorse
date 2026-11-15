// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EasyMorse

// Licenses of EasyMorse and of the software it ships; a line opens to show
// its copyright and the full text of its license
Popup {
    id: dialog

    // Line shown open, -1 for none
    property int expanded: -1

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent.width - 24, 760)
    height: parent.height - 24
    modal: true
    focus: true
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    onOpened: {
        expanded = -1
        list.positionViewAtBeginning()
    }

    function groupTitle(group) {
        switch (group) {
        case "Libraries": return qsTr("Libraries")
        case "Fonts": return qsTr("Fonts")
        case "Inside Qt": return qsTr("Code carried by Qt")
        }
        return group
    }

    background: Rectangle {
        radius: Theme.radius
        color: Theme.surface
    }

    contentItem: ColumnLayout {
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 8
            Layout.topMargin: 8

            Label {
                text: qsTr("Licenses")
                font.pixelSize: 18
                font.weight: Font.DemiBold
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("Close")
                flat: true
                onClicked: dialog.close()
            }
        }

        Label {
            text: qsTr("EasyMorse is free software under the GNU General Public License version 3 or later. "
                       + "It ships the software below, each under its own license. Open a line to read its license.")
            color: Theme.muted
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.bottomMargin: 8
        }

        ListView {
            id: list
            model: Licenses.components
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            Layout.fillWidth: true
            Layout.fillHeight: true
            ScrollBar.vertical: ScrollBar {}

            delegate: Column {
                id: row

                required property var modelData
                required property int index
                readonly property bool open: dialog.expanded === index
                readonly property bool firstOfGroup: index === 0
                    || Licenses.components[index - 1].group !== modelData.group

                width: ListView.view.width

                Label {
                    visible: row.firstOfGroup && row.index > 0
                    x: 20
                    width: row.width - 40
                    topPadding: 16
                    bottomPadding: 4
                    text: dialog.groupTitle(row.modelData.group).toUpperCase()
                    color: Theme.muted
                    elide: Text.ElideRight
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    font.letterSpacing: 1.2
                }

                ItemDelegate {
                    width: row.width
                    leftPadding: 20
                    rightPadding: 20
                    onClicked: dialog.expanded = row.open ? -1 : row.index

                    contentItem: ColumnLayout {
                        spacing: 2

                        Label {
                            // EasyMorse itself: the version that runs
                            text: row.modelData.name + " "
                                  + (row.index === 0 ? Qt.application.version : (row.modelData.version || ""))
                            font.weight: Font.Medium
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }
                        Label {
                            text: [row.modelData.license || qsTr("Terms of its publisher"),
                                   row.modelData.platforms ? qsTr("only on %1").arg(row.modelData.platforms) : ""]
                                  .filter((part) => part.length > 0).join(" · ")
                            color: Theme.muted
                            font.pixelSize: 12
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }
                    }
                }

                Column {
                    visible: row.open
                    x: 20
                    width: row.width - 40
                    bottomPadding: 16
                    spacing: 10

                    Label {
                        visible: text.length > 0
                        width: parent.width
                        text: row.modelData.usage || ""
                        wrapMode: Text.Wrap
                    }
                    Label {
                        visible: text.length > 0
                        width: parent.width
                        text: row.modelData.copyright || ""
                        textFormat: Text.PlainText
                        wrapMode: Text.Wrap
                    }
                    Label {
                        visible: (row.modelData.url || "").length > 0
                        width: parent.width
                        text: "<a href=\"" + row.modelData.url + "\">" + row.modelData.url + "</a>"
                        textFormat: Text.StyledText
                        linkColor: Theme.accentText
                        wrapMode: Text.WrapAnywhere
                        onLinkActivated: (link) => Qt.openUrlExternally(link)
                    }
                    Label {
                        visible: text.length > 0
                        width: parent.width
                        // Read only once the line is open: some texts are long
                        text: row.open ? Licenses.licenseText(row.modelData.license || "") : ""
                        textFormat: Text.PlainText
                        wrapMode: Text.Wrap
                        color: Theme.muted
                        font.family: Theme.monoFont
                        font.pixelSize: 11
                    }
                }
            }
        }
    }
}
