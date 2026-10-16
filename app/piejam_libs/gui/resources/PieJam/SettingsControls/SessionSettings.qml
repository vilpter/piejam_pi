// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import PieJam.Controls 1.0
import PieJam.Dialogs 1.0
import PieJam.Models 1.0 as PJModels

SubscribableItem {
    id: root

    property PJModels.SessionSettings model: null

    QtObject {
        id: private_

        readonly property int newSessionType:
            root.model ? root.model.newSessionType : PJModels.SessionSettings.NewSessionType.Empty
        readonly property string currentSession: root.model ? root.model.currentSession : ""
        readonly property bool isSessionModified: root.model && root.model.isSessionModified
        property bool openingUnsavedChanges: false
        property bool newSessionUnsavedChanges: false

        function newSession() {
            private_.openingUnsavedChanges = false
            private_.newSessionUnsavedChanges = private_.isSessionModified

            if (private_.newSessionUnsavedChanges) {
                unsavedChangesDialog.open()
            } else {
                root.model.newSession()
            }
        }

        function showOpen() {
            private_.openingUnsavedChanges = private_.isSessionModified
            private_.newSessionUnsavedChanges = false

            if (private_.openingUnsavedChanges) {
                unsavedChangesDialog.open()
            } else {
                openFileDialog.open()
            }
        }

        function showSave() {
            private_.openingUnsavedChanges = false
            private_.newSessionUnsavedChanges = false

            saveFileDialog.open()
        }
    }

    YesNoCancelDialog {
        id: unsavedChangesDialog

        title: qsTr("Unsaved Session changes")

        message: qsTr("You have unsaved Session changes. Do you want to save them?")

        onYesClicked: {
            if (private_.currentSession !== "") {
                root.model.saveCurrentSession()

                if (private_.openingUnsavedChanges) {
                    openFileDialog.open()
                } else if (private_.newSessionUnsavedChanges) {
                    root.model.newSession()
                }
            } else {
                saveFileDialog.open()
            }
        }

        onNoClicked: {
            if (private_.openingUnsavedChanges) {
                openFileDialog.open()
            } else if (private_.newSessionUnsavedChanges) {
                root.model.newSession()
            }
        }
    }

    YesNoCancelDialog {
        id: overwriteDialog

        title: qsTr("Overwrite Session")
        message: qsTr("File already exists. Do you want to overwrite it?")

        onYesClicked: {
            root.model.saveSession(saveFileDialog.selectedPath)
            saveFileDialog.resetSelection()

            if (private_.newSessionUnsavedChanges) {
                root.model.newSession()
            }
        }

        onNoClicked: {
            saveFileDialog.open()
        }
    }

    OpenFileDialog {
        id: openFileDialog

        model: root.model.sessionFileDialog
        title: qsTr("Open Session")

        onSelected: {
            root.model.openSession(openFileDialog.selectedPath)
            openFileDialog.resetSelection()
        }
    }

    SaveFileDialog {
        id: saveFileDialog

        model: root.model.sessionFileDialog
        title: qsTr("Save Session")

        onSelected: {
            if (PJModels.Filesystem.fileExists(saveFileDialog.selectedPath)) {
                overwriteDialog.open()
            } else if (private_.openingUnsavedChanges) {
                root.model.saveSession(saveFileDialog.selectedPath)
                saveFileDialog.resetSelection()
                openFileDialog.open()
            } else if (private_.newSessionUnsavedChanges) {
                root.model.saveSession(saveFileDialog.selectedPath)
                saveFileDialog.resetSelection()
                root.model.newSession()
            } else {
                root.model.saveSession(saveFileDialog.selectedPath)
                saveFileDialog.resetSelection()
            }
        }
    }

    OpenFileDialog {
        id: sessionTemplateFileDialog

        model: root.model.sessionFileDialog
        title: qsTr("Select Session template")

        onSelected: {
            root.model.setTemplateFromSession(sessionTemplateFileDialog.selectedPath)
            sessionTemplateFileDialog.resetSelection()
        }
    }

    ButtonGroup {
        id: startupButtons
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        Frame {
            Layout.fillWidth: true
            Layout.preferredHeight: 144

            spacing: 0

            ColumnLayout {
                anchors.fill: parent

                Label {
                    Layout.fillWidth: true

                    textFormat: Text.PlainText
                    font.pixelSize: 18

                    text: qsTr("New session")
                }

                RowLayout {
                    Layout.fillWidth: true

                    Button {
                        text: "New"

                        onClicked: private_.newSession()
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    ChoiceBox {
                        displayText: "Template"
                        popup: Menu {
                            width: 300

                            MenuItem {
                                text: "Set from file..."

                                onClicked: sessionTemplateFileDialog.open()
                            }

                            MenuItem {
                                text: "Use current session"

                                onClicked: root.model.setTemplateFromCurrent()
                            }

                            MenuItem {
                                text: "Empty session"

                                onClicked: root.model.setEmptySessionTemplate()
                            }
                        }
                    }
                }

                RadioButton {
                    ButtonGroup.group: startupButtons

                    text: "Startup session"

                    checked: root.model && root.model.startupSession === PJModels.SessionSettings.StartupSession.New

                    onClicked: root.model.switchStartupSession(PJModels.SessionSettings.StartupSession.New)
                }
            }
        }

        Frame {
            Layout.fillWidth: true
            Layout.preferredHeight: 144

            spacing: 0

            ColumnLayout {
                anchors.fill: parent

                Label {
                    Layout.fillWidth: true

                    textFormat: Text.PlainText
                    font.pixelSize: 18

                    text: qsTr("Current session") + " [" + private_.currentSession + "]"
                }

                RowLayout {
                    Layout.fillWidth: true

                    Button {
                        text: "Open..."

                        onClicked: private_.showOpen()
                    }

                    Button {
                        text: "Save"

                        enabled: private_.currentSession === "" || private_.isSessionModified

                        onClicked: {
                            if (private_.currentSession !== "") {
                                root.model.saveCurrentSession()
                            } else {
                                private_.showSave()
                            }
                        }
                    }

                    Button {
                        text: "Save As..."

                        onClicked: private_.showSave()
                    }
                }

                RadioButton {
                    ButtonGroup.group: startupButtons

                    text: "Startup session"

                    checked: root.model && root.model.startupSession === PJModels.SessionSettings.StartupSession.Last

                    onClicked: root.model.switchStartupSession(PJModels.SessionSettings.StartupSession.Last)
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
