// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: root

    property alias mountName: nameField.text
    property alias serverHost: serverField.text
    property alias remotePath: remotePathField.text
    property alias localMount: localMountField.text
    property alias nfsVersion: versionCombo.currentIndex  // 0=v3, 1=v4
    property alias readWrite: readWriteSwitch.checked
    property alias autoMount: autoMountSwitch.checked

    signal testConnection(string serverHost, string remotePath)
    signal fetchExports(string serverHost)

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: 400

    modal: true
    focus: true

    standardButtons: Dialog.Cancel | Dialog.Ok

    onOpened: {
        nameField.forceActiveFocus()
    }

    onAccepted: {
        // Validation handled by caller
    }

    onRejected: {
        reset()
    }

    function reset() {
        nameField.text = ""
        serverField.text = ""
        remotePathField.text = ""
        localMountField.text = ""
        versionCombo.currentIndex = 1  // Default to NFSv4
        readWriteSwitch.checked = true
        autoMountSwitch.checked = false
        testResultLabel.visible = false
        exportsCombo.model = []
    }

    function testResult(success) {
        testResultLabel.text = success
            ? qsTr("Connection successful")
            : qsTr("Connection failed")
        testResultLabel.color = success
            ? Material.color(Material.Green)
            : Material.color(Material.Red)
        testResultLabel.visible = true
    }

    function exportsReceived(exports) {
        exportsCombo.model = exports
        exportsCombo.visible = exports.length > 0
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        // Name
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Label {
                text: qsTr("Name")
            }

            TextField {
                id: nameField
                Layout.fillWidth: true
                placeholderText: qsTr("e.g., Studio NAS")
            }
        }

        // Server Host
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Label {
                text: qsTr("Server IP / Hostname")
            }

            RowLayout {
                Layout.fillWidth: true

                TextField {
                    id: serverField
                    Layout.fillWidth: true
                    placeholderText: qsTr("e.g., 192.168.1.100 or nas.local")
                    inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                }

                Button {
                    text: qsTr("Browse")
                    enabled: serverField.text.length > 0
                    onClicked: {
                        root.fetchExports(serverField.text)
                    }
                }
            }
        }

        // Available exports (populated after Browse)
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            visible: exportsCombo.model && exportsCombo.model.length > 0

            Label {
                text: qsTr("Available Exports")
            }

            ComboBox {
                id: exportsCombo
                Layout.fillWidth: true
                model: []
                onActivated: {
                    remotePathField.text = currentText
                }
            }
        }

        // Remote Path
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Label {
                text: qsTr("Remote Export Path")
            }

            TextField {
                id: remotePathField
                Layout.fillWidth: true
                placeholderText: qsTr("e.g., /export/audio")
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
            }
        }

        // Local Mount Point
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Label {
                text: qsTr("Local Mount Point")
            }

            TextField {
                id: localMountField
                Layout.fillWidth: true
                placeholderText: qsTr("e.g., /mnt/nas_audio (auto-generated if empty)")
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
            }
        }

        // NFS Version
        RowLayout {
            Layout.fillWidth: true

            Label {
                text: qsTr("NFS Version")
                Layout.fillWidth: true
            }

            ComboBox {
                id: versionCombo
                model: ["NFSv3", "NFSv4"]
                currentIndex: 1  // Default to NFSv4
            }
        }

        // Read-Write
        RowLayout {
            Layout.fillWidth: true

            Label {
                text: qsTr("Read-Write Access")
                Layout.fillWidth: true
            }

            Switch {
                id: readWriteSwitch
                checked: true
            }
        }

        // Auto-mount
        RowLayout {
            Layout.fillWidth: true

            Label {
                text: qsTr("Auto-mount on Boot")
                Layout.fillWidth: true
            }

            Switch {
                id: autoMountSwitch
                checked: false
            }
        }

        // Test connection button
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: qsTr("Test Connection")
                enabled: serverField.text.length > 0

                onClicked: {
                    testResultLabel.text = qsTr("Testing...")
                    testResultLabel.color = Material.foreground
                    testResultLabel.visible = true
                    root.testConnection(serverField.text, remotePathField.text)
                }
            }

            Label {
                id: testResultLabel
                visible: false
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }
}
