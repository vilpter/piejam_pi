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
                text: qsTr("NFS Client Mounts")
                font.pixelSize: 18
                font.bold: true
                Layout.fillWidth: true
            }

            Button {
                text: qsTr("Add Mount")
                enabled: root.model && root.model.nfsClientAvailable
                onClicked: {
                    addMountDialog.reset()
                    addMountDialog.open()
                }
            }
        }

        // Availability notice
        Label {
            Layout.fillWidth: true
            text: qsTr("NFS client utilities not installed")
            visible: root.model && !root.model.nfsClientAvailable
            color: Material.color(Material.Orange)
            wrapMode: Text.WordWrap
        }

        // Mounts list
        ListView {
            id: mountsList

            Layout.fillWidth: true
            Layout.fillHeight: true

            clip: true
            model: root.model ? root.model.nfsMounts() : null
            spacing: 8

            delegate: NFSMountDelegate {
                width: mountsList.width
                name: model.name
                serverHost: model.serverHost
                remotePath: model.remotePath
                localMount: model.localMount
                statusString: model.statusString
                isMounted: model.status === 2  // nfs_mount_status::mounted
                availableSpace: model.availableSpace
                totalSpace: model.totalSpace
                autoMount: model.autoMount

                onMountClicked: {
                    if (root.model) {
                        root.model.mountNfs(model.configId)
                    }
                }

                onUnmountClicked: {
                    if (root.model) {
                        root.model.unmountNfs(model.configId)
                    }
                }

                onRemoveClicked: {
                    removeConfirmDialog.mountId = model.configId
                    removeConfirmDialog.mountName = model.name
                    removeConfirmDialog.open()
                }

                onEditClicked: {
                    editMountDialog.configId = model.configId
                    editMountDialog.mountName = model.name
                    editMountDialog.serverHost = model.serverHost
                    editMountDialog.remotePath = model.remotePath
                    editMountDialog.localMount = model.localMount
                    editMountDialog.nfsVersion = model.version
                    editMountDialog.readWrite = model.readWrite
                    editMountDialog.autoMount = model.autoMount
                    editMountDialog.open()
                }
            }

            ScrollBar.vertical: ScrollBar {}

            Label {
                anchors.centerIn: parent
                text: qsTr("No NFS mounts configured.\nTap 'Add Mount' to add one.")
                horizontalAlignment: Text.AlignHCenter
                visible: mountsList.count === 0
                opacity: 0.5
            }
        }

        // Quick actions
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: root.model && root.model.nfsClientAvailable

            Button {
                text: qsTr("Refresh")
                onClicked: {
                    if (root.model) {
                        root.model.refreshStatus()
                    }
                }
            }

            Button {
                text: qsTr("Unmount All")
                onClicked: {
                    if (root.model) {
                        root.model.unmountAllNfs()
                    }
                }
            }
        }
    }

    // Add Mount Dialog
    NFSMountDialog {
        id: addMountDialog

        title: qsTr("Add NFS Mount")

        onAccepted: {
            if (root.model) {
                root.model.addNfsMount(
                    mountName,
                    serverHost,
                    remotePath,
                    localMount,
                    nfsVersion === 1,  // true if NFSv4
                    readWrite,
                    autoMount
                )
            }
        }

        onTestConnection: {
            if (root.model) {
                var success = root.model.testNfsConnection(serverHost, remotePath)
                testResult(success)
            }
        }

        onFetchExports: {
            if (root.model) {
                var exports = root.model.getNfsServerExports(serverHost)
                exportsReceived(exports)
            }
        }
    }

    // Edit Mount Dialog
    NFSMountDialog {
        id: editMountDialog

        property string configId: ""

        title: qsTr("Edit NFS Mount")

        onAccepted: {
            if (root.model && configId !== "") {
                root.model.updateNfsMount(
                    configId,
                    mountName,
                    serverHost,
                    remotePath,
                    localMount,
                    nfsVersion === 1,  // true if NFSv4
                    readWrite,
                    autoMount
                )
            }
        }

        onTestConnection: {
            if (root.model) {
                var success = root.model.testNfsConnection(serverHost, remotePath)
                testResult(success)
            }
        }

        onFetchExports: {
            if (root.model) {
                var exports = root.model.getNfsServerExports(serverHost)
                exportsReceived(exports)
            }
        }
    }

    // Remove Confirmation Dialog
    Dialog {
        id: removeConfirmDialog

        property string mountId: ""
        property string mountName: ""

        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 400
        title: qsTr("Remove NFS Mount")
        standardButtons: Dialog.Yes | Dialog.No

        Label {
            text: qsTr("Remove mount '%1'?\n\nThis will unmount if currently mounted.").arg(removeConfirmDialog.mountName)
            wrapMode: Text.WordWrap
        }

        onAccepted: {
            if (root.model && mountId !== "") {
                root.model.removeNfsMount(mountId)
            }
        }
    }

    // Handle signals from model
    Connections {
        target: root.model

        function onNfsMountStatusChanged(id, status) {
            // Status updated automatically via model
        }

        function onNfsMountError(id, message) {
            mountErrorDialog.text = message
            mountErrorDialog.open()
        }
    }

    // Error Dialog
    Dialog {
        id: mountErrorDialog

        property alias text: errorLabel.text

        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 400
        title: qsTr("NFS Mount Error")
        standardButtons: Dialog.Ok

        Label {
            id: errorLabel
            wrapMode: Text.WordWrap
        }
    }
}
