// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root

    property string name: ""
    property string serverHost: ""
    property string remotePath: ""
    property string localMount: ""
    property string statusString: ""
    property bool isMounted: false
    property real availableSpace: 0
    property real totalSpace: 0
    property bool autoMount: false

    signal mountClicked()
    signal unmountClicked()
    signal removeClicked()
    signal editClicked()

    implicitHeight: contentColumn.implicitHeight + 16
    color: Material.color(Material.Grey, Material.Shade900)
    radius: 4

    ColumnLayout {
        id: contentColumn

        anchors.fill: parent
        anchors.margins: 8
        spacing: 4

        // Header row: Name + Status
        RowLayout {
            Layout.fillWidth: true

            Label {
                text: root.name
                font.pixelSize: 16
                font.bold: true
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            Rectangle {
                width: statusLabel.implicitWidth + 12
                height: statusLabel.implicitHeight + 4
                radius: 2
                color: root.isMounted
                       ? Material.color(Material.Green, Material.Shade700)
                       : Material.color(Material.Grey, Material.Shade700)

                Label {
                    id: statusLabel
                    anchors.centerIn: parent
                    text: root.statusString
                    font.pixelSize: 11
                }
            }

            Label {
                text: qsTr("Auto")
                font.pixelSize: 11
                opacity: 0.6
                visible: root.autoMount
            }
        }

        // Server info
        Label {
            text: root.serverHost + ":" + root.remotePath
            font.pixelSize: 12
            opacity: 0.7
            elide: Text.ElideMiddle
            Layout.fillWidth: true
        }

        // Local mount point
        Label {
            text: qsTr("Mount: %1").arg(root.localMount)
            font.pixelSize: 11
            opacity: 0.5
        }

        // Space info (if mounted)
        RowLayout {
            Layout.fillWidth: true
            visible: root.isMounted && root.totalSpace > 0

            ProgressBar {
                Layout.preferredWidth: 100
                from: 0
                to: root.totalSpace
                value: root.totalSpace - root.availableSpace
            }

            Label {
                text: formatBytes(root.availableSpace) + " free"
                font.pixelSize: 11
                opacity: 0.7
            }
        }

        // Action buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: root.isMounted ? qsTr("Unmount") : qsTr("Mount")
                flat: true
                font.pixelSize: 12
                onClicked: {
                    if (root.isMounted) {
                        root.unmountClicked()
                    } else {
                        root.mountClicked()
                    }
                }
            }

            Button {
                text: qsTr("Edit")
                flat: true
                font.pixelSize: 12
                enabled: !root.isMounted
                onClicked: root.editClicked()
            }

            Item { Layout.fillWidth: true }

            Button {
                text: qsTr("Remove")
                flat: true
                font.pixelSize: 12
                Material.foreground: Material.color(Material.Red)
                onClicked: root.removeClicked()
            }
        }
    }

    function formatBytes(bytes) {
        if (bytes === 0) return "0 B"
        var k = 1024
        var sizes = ["B", "KB", "MB", "GB", "TB"]
        var i = Math.floor(Math.log(bytes) / Math.log(k))
        return parseFloat((bytes / Math.pow(k, i)).toFixed(1)) + " " + sizes[i]
    }
}
