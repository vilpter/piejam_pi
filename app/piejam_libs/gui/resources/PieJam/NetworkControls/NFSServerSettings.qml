// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

Frame {
    id: root

    property var model: null

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        // Header
        RowLayout {
            Layout.fillWidth: true

            Label {
                text: qsTr("NFS Server")
                font.pixelSize: 18
                font.bold: true
                Layout.fillWidth: true
            }

            Label {
                text: root.model && root.model.nfsServerActive
                      ? qsTr("Active")
                      : qsTr("Inactive")
                color: root.model && root.model.nfsServerActive
                       ? Material.color(Material.Green)
                       : Material.foreground
                font.bold: true
            }
        }

        // Availability notice
        Label {
            Layout.fillWidth: true
            text: qsTr("NFS server packages not installed")
            visible: root.model && !root.model.nfsServerAvailable
            color: Material.color(Material.Orange)
            wrapMode: Text.WordWrap
        }

        // Enable/Disable toggle
        RowLayout {
            Layout.fillWidth: true
            visible: root.model && root.model.nfsServerAvailable

            Label {
                text: qsTr("Enable NFS Server")
                Layout.fillWidth: true
            }

            Switch {
                checked: root.model ? root.model.nfsServerActive : false
                onToggled: {
                    if (root.model) {
                        root.model.toggleNfsServer()
                    }
                }
            }
        }

        // Export path (read-only)
        RowLayout {
            Layout.fillWidth: true
            visible: root.model && root.model.nfsServerAvailable

            Label {
                text: qsTr("Export Path:")
                font.bold: true
            }

            Label {
                text: root.model ? root.model.nfsExportPath : "/home/piejam/recordings"
                opacity: 0.8
            }
        }

        // Read/Write toggle
        RowLayout {
            Layout.fillWidth: true
            visible: root.model && root.model.nfsServerAvailable

            Label {
                text: qsTr("Read-Write Access")
                Layout.fillWidth: true
            }

            Switch {
                checked: root.model ? root.model.nfsServerReadWrite : true
                onToggled: {
                    if (root.model) {
                        root.model.setNfsServerReadWriteMode(checked)
                    }
                }
            }
        }

        // Local IP addresses (when active)
        ColumnLayout {
            Layout.fillWidth: true
            visible: root.model && root.model.nfsServerActive && root.model.nfsLocalIpAddresses.length > 0
            spacing: 4

            Label {
                text: qsTr("Server IP Address(es):")
                font.bold: true
            }

            Repeater {
                model: root.model ? root.model.nfsLocalIpAddresses : []
                Label {
                    text: modelData
                    font.family: "monospace"
                    opacity: 0.8
                }
            }
        }

        // Mount command display (when active)
        ColumnLayout {
            Layout.fillWidth: true
            visible: root.model && root.model.nfsServerActive
            spacing: 4

            Label {
                text: qsTr("Mount from another device:")
                font.bold: true
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: mountCommandText.implicitHeight + 16
                color: Material.color(Material.Grey, Material.Shade900)
                radius: 4

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8

                    Label {
                        id: mountCommandText
                        text: root.model ? root.model.nfsMountCommand : ""
                        font.family: "monospace"
                        font.pixelSize: 12
                        Layout.fillWidth: true
                        wrapMode: Text.WrapAnywhere
                    }
                }
            }
        }

        // Actions
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: root.model && root.model.nfsServerAvailable

            Button {
                text: qsTr("Test Connection")
                enabled: root.model && root.model.nfsServerActive
                onClicked: {
                    if (root.model) {
                        var success = root.model.testNfsServer()
                        testResultLabel.text = success
                            ? qsTr("Server is accessible")
                            : qsTr("Test failed")
                        testResultLabel.color = success
                            ? Material.color(Material.Green)
                            : Material.color(Material.Red)
                        testResultLabel.visible = true
                    }
                }
            }

            Item { Layout.fillWidth: true }

            Label {
                id: testResultLabel
                visible: false
            }
        }

        // Spacer
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    // Handle signals from model
    Connections {
        target: root.model

        function onNfsServerStatusChanged(active) {
            // Status updated automatically via property binding
        }

        function onNfsServerError(message) {
            serverErrorDialog.text = message
            serverErrorDialog.open()
        }
    }

    // Error Dialog
    Dialog {
        id: serverErrorDialog

        property alias text: errorLabel.text

        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 400
        title: qsTr("NFS Server Error")
        standardButtons: Dialog.Ok

        Label {
            id: errorLabel
            wrapMode: Text.WordWrap
        }
    }
}
