// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import PieJam.Models 1.0 as PJModels

Item {
    id: root

    property PJModels.FileDialog model: null
    property alias title: dialog.title
    property bool allowFileNameEditing: false
    property var selectedPath: null
    property string selectButtonText: qsTr("Select")

    signal canceled()
    signal selected()

    Dialog {
        id: dialog

        property PJModels.FileDialog model: null

        width: Overlay.overlay.width
        height: Overlay.overlay.height

        parent: Overlay.overlay
        anchors.centerIn: Overlay.overlay

        modal: true

        title: qsTr("Select")

        bottomPadding: 0

        ColumnLayout {
            anchors.fill: parent

            RowLayout {
                Layout.fillWidth: true

                Button {
                    icon.source: "qrc:///images/icons/parent_folder.svg"

                    enabled: root.model && root.model.canGoUp

                    onClicked: {
                        root.model.goUp()
                        filesList.resetSelection()
                    }
                }

                Button {
                    icon.source: "qrc:///images/icons/new_folder.svg"

                    enabled: root.model

                    onClicked: newFolderName.focus = true
                }

                TextField {
                    id: newFolderName

                    enabled: root.model
                    visible: false

                    onEditingFinished: {
                        console.assert(root.model)

                        newFolderName.focus = false

                        if (newFolderName.text !== "") {
                            root.model.createDirectory(newFolderName.text)
                            newFolderName.clear()
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    icon.source: "qrc:///images/icons/delete_forever.svg"

                    enabled: filesList.currentEntry !== null

                    onClicked: {
                        console.assert(root.model)
                        console.assert(filesList.currentEntry !== null)

                        root.model.remove(filesList.currentIndex)
                        filesList.resetSelection()
                    }
                }
            }

            Frame {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ListView {
                    id: filesList

                    property var currentEntry: null

                    anchors.fill: parent

                    model: root.model ? root.model.entries : null

                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    boundsMovement: Flickable.StopAtBounds
                    reuseItems: true

                    delegate: ItemDelegate {
                        width: filesList.width

                        text: model.text

                        icon.source: {
                            switch (model.value.type) {
                            case PJModels.FileDialogEntry.Type.Directory:
                                return "qrc:/images/icons/folder.svg"
                            case PJModels.FileDialogEntry.Type.File:
                                return "qrc:/images/icons/file.svg"
                            default:
                                return ""
                            }
                        }

                        font.capitalization: Font.MixedCase
                        highlighted: ListView.isCurrentItem

                        onClicked: {
                            filesList.currentIndex = index
                            filesList.currentEntry = model.value
                            fileNameField.text = model.value.type === PJModels.FileDialogEntry.Type.File ? model.text : ""
                        }

                        onDoubleClicked: {
                            if (model.value.type === PJModels.FileDialogEntry.Type.Directory) {
                                root.model.changeDirectory(index)
                                filesList.resetSelection()
                            }
                        }
                    }

                    onModelChanged: filesList.resetSelection()

                    function resetSelection() {
                        filesList.currentIndex = -1
                        filesList.currentEntry = null
                    }
                }
            }

            TextField {
                id: fileNameField

                Layout.fillWidth: true

                enabled: root.allowFileNameEditing && root.model

                placeholderText: qsTr("<File name>")
            }
        }

        footer: Item {
            implicitHeight: 64

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 24
                anchors.rightMargin: 24
                anchors.bottomMargin: 24

                Button {
                    text: qsTr("Cancel")

                    onClicked: {
                        dialog.close()
                        root.resetSelection()
                        root.canceled()
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    text: root.selectButtonText

                    enabled: root.model && canSelect(fileNameField.text)

                    onClicked: {
                        dialog.close()
                        root.selectedPath = root.model.selectFile(fileNameField.text)
                        root.selected()
                    }

                    function canSelect(name) {
                        console.assert(root.model)

                        if (name === "")
                            return false

                        var path = root.model.selectFile(name)
                        return !PJModels.Filesystem.fileExists(path) || PJModels.Filesystem.isRegularFile(path)
                    }
                }
            }
        }
    }

    function open() {
        if (root.model) {
            root.model.updateEntries()
        }

        dialog.open()
    }

    function resetSelection() {
        filesList.resetSelection()
        fileNameField.text = ""
        root.selectedPath = null
    }
}
