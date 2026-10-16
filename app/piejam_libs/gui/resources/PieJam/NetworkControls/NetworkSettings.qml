// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import PieJam.Controls 1.0
import PieJam.Models 1.0 as PJModels

SubscribableItem {
    id: root

    property var model: null

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        // Compact header: enable toggle + status + action buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            Label {
                text: qsTr("Network")
                font.pixelSize: 20
                font.bold: true
            }

            Switch {
                id: networkSwitch
                checked: root.model ? root.model.networkEnabled : false
                onToggled: {
                    if (root.model) {
                        root.model.toggleNetwork()
                    }
                }
            }

            Item { Layout.fillWidth: true }

            // Connection status - show IP when connected
            Label {
                text: {
                    if (!root.model)
                        return ""
                    if (root.model.wifiConnected && root.model.ipAddress !== "")
                        return root.model.ipAddress
                    if (root.model.isConnecting
                            || (root.model.wifiConnected
                                && root.model.ipAddress === ""))
                        return qsTr("Connecting...")
                    if (root.model.networkEnabled && !root.model.wifiConnected)
                        return qsTr("NO WIFI CONNECTION")
                    return qsTr("Disconnected")
                }
                color: {
                    if (root.model && root.model.wifiConnected
                            && root.model.ipAddress !== "")
                        return Material.color(Material.Green)
                    if (root.model && root.model.networkEnabled
                            && !root.model.wifiConnected)
                        return Material.color(Material.Red)
                    return Material.foreground
                }
            }

            // Signal strength (compact)
            ProgressBar {
                Layout.preferredWidth: 60
                from: 0
                to: 100
                value: root.model ? root.model.signalStrength : 0
                visible: root.model && root.model.wifiConnected
            }
        }

        // Action buttons row
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: qsTr("Scan")
                flat: true
                enabled: root.model && root.model.networkEnabled && !root.model.isScanning
                onClicked: {
                    if (root.model) {
                        root.model.scanNetworks()
                    }
                }

                BusyIndicator {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 4
                    width: 20
                    height: 20
                    running: root.model && root.model.isScanning
                    visible: running
                }
            }

            Button {
                text: qsTr("Disconnect")
                flat: true
                visible: root.model && root.model.wifiConnected
                onClicked: {
                    if (root.model) {
                        root.model.disconnectWifi()
                    }
                }
            }

            Button {
                text: qsTr("Saved Networks")
                flat: true
                enabled: root.model && root.model.networkEnabled
                onClicked: savedNetworksPopup.open()
            }

            Item { Layout.fillWidth: true }

            Switch {
                id: nfsSwitch
                checked: root.model ? root.model.nfsServerActive : false
                enabled: root.model && root.model.wifiConnected
                onToggled: {
                    if (root.model) root.model.toggleNfsServer()
                }
            }

            Label {
                text: qsTr("NFS Server")
                font.pixelSize: 12
                opacity: 0.7
            }

            Label {
                visible: root.model && root.model.nfsServerActive
                text: root.model
                      ? "NFS: " + root.model.ipAddress + ":"
                        + root.model.nfsExportPath
                      : ""
                font.pixelSize: 12
                color: Material.color(Material.Green)
            }
        }

        // Available Networks list - gets all remaining vertical space
        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: networkList

                anchors.fill: parent
                clip: true
                model: root.model ? root.model.availableNetworks() : null

                delegate: WiFiNetworkDelegate {
                    width: networkList.width
                    ssid: model.ssid
                    signalPercent: model.signalPercent
                    security: model.security
                    band: model.band
                    isConnected: model.isConnected

                    onClicked: {
                        if (!isConnected) {
                            connectionDialog.ssid = ssid
                            connectionDialog.securityType = security
                            connectionDialog.open()
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar {}

                Label {
                    anchors.centerIn: parent
                    text: root.model && root.model.isScanning
                          ? qsTr("Scanning...")
                          : qsTr("No networks found.\nTap 'Scan' to search.")
                    horizontalAlignment: Text.AlignHCenter
                    visible: networkList.count === 0
                    opacity: 0.5
                }
            }
        }
    }

    // Connection Dialog
    WiFiConnectionDialog {
        id: connectionDialog

        onAccepted: {
            if (root.model) {
                root.model.connectToNetwork(ssid, password, rememberNetwork)
            }
        }
    }

    // Saved Networks Popup
    Popup {
        id: savedNetworksPopup

        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 400
        height: 300

        modal: true
        focus: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            Label {
                text: qsTr("Saved Networks")
                font.pixelSize: 18
                font.bold: true
            }

            ListView {
                id: savedNetworkList

                Layout.fillWidth: true
                Layout.fillHeight: true

                clip: true
                model: root.model ? root.model.savedNetworks() : null

                delegate: ItemDelegate {
                    width: savedNetworkList.width
                    text: model.ssid

                    RowLayout {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.rightMargin: 8

                        Label {
                            text: model.security
                            opacity: 0.6
                        }

                        ToolButton {
                            text: qsTr("Connect")
                            font.pixelSize: 12
                            onClicked: {
                                if (root.model) {
                                    savedNetworksPopup.close()
                                    root.model.connectToNetwork(model.ssid, "", false)
                                }
                            }
                        }

                        ToolButton {
                            text: "X"
                            onClicked: {
                                if (root.model) {
                                    root.model.forgetNetwork(model.ssid)
                                }
                            }
                        }
                    }
                }

                Label {
                    anchors.centerIn: parent
                    text: qsTr("No saved networks")
                    visible: savedNetworkList.count === 0
                    opacity: 0.5
                }
            }

            Button {
                Layout.alignment: Qt.AlignRight
                text: qsTr("Close")
                onClicked: savedNetworksPopup.close()
            }
        }
    }

    // Handle signals from model
    Connections {
        target: root.model

        function onConnectionSucceeded(ssid) {
            // Could show a toast/notification
        }

        function onConnectionFailed(ssid, error) {
            errorDialog.text = qsTr("Failed to connect to %1:\n%2").arg(ssid).arg(error)
            errorDialog.open()
        }

        function onNetworkError(message) {
            errorDialog.text = message
            errorDialog.open()
        }
    }

    // Error Dialog
    Dialog {
        id: errorDialog

        property alias text: errorLabel.text

        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 400
        title: qsTr("Network Error")
        standardButtons: Dialog.Ok

        Label {
            id: errorLabel
            wrapMode: Text.WordWrap
        }
    }
}
