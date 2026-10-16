// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

ItemDelegate {
    id: root

    property string ssid: ""
    property int signalPercent: 0
    property string security: ""
    property string band: ""
    property bool isConnected: false

    implicitHeight: 56

    contentItem: RowLayout {
        spacing: 12

        // Signal strength indicator
        Item {
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24

            // Draw WiFi signal bars
            Row {
                anchors.centerIn: parent
                spacing: 2

                Repeater {
                    model: 4

                    Rectangle {
                        width: 4
                        height: 6 + index * 4
                        anchors.bottom: parent.bottom
                        radius: 1
                        color: {
                            var threshold = (index + 1) * 25
                            return root.signalPercent >= threshold
                                   ? Material.accent
                                   : Material.color(Material.Grey, Material.Shade600)
                        }
                    }
                }
            }
        }

        // Network info
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            RowLayout {
                spacing: 8

                Label {
                    text: root.ssid
                    font.pixelSize: 16
                    font.bold: root.isConnected
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("Connected")
                    font.pixelSize: 12
                    color: Material.color(Material.Green)
                    visible: root.isConnected
                }
            }

            RowLayout {
                spacing: 8

                Label {
                    text: root.security
                    font.pixelSize: 12
                    opacity: 0.7
                    visible: root.security !== "Open"
                }

                Label {
                    text: root.band
                    font.pixelSize: 12
                    opacity: 0.7
                    visible: root.band !== "" && root.band !== "Unknown"
                }

                Label {
                    text: root.signalPercent + "%"
                    font.pixelSize: 12
                    opacity: 0.7
                }
            }
        }
    }

    background: Rectangle {
        color: root.highlighted || root.down
               ? Material.color(Material.Grey, Material.Shade800)
               : (root.hovered
                  ? Material.color(Material.Grey, Material.Shade900)
                  : "transparent")

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Material.color(Material.Grey, Material.Shade800)
        }
    }
}
