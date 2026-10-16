// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import PieJam.Controls 1.0
import PieJam.Models 1.0 as PJModels

SubscribableItem {
    id: root

    property PJModels.ExternalAudioDeviceConfig model: null

    property var channels: [ "-" ]

    RowLayout {
        anchors.fill: parent
        anchors.margins: 4

        StringTextField {
            Layout.fillWidth: true

            model: root.model ? root.model.name : null
        }

        StackLayout {
            Layout.maximumWidth: 128
            Layout.topMargin: 4
            Layout.bottomMargin: 4

            currentIndex: root.model ? (root.model.mono ? 0 : 1) : -1

            ChannelComboBox {
                Layout.fillWidth: true
                Layout.fillHeight: true

                channels: root.channels
                channel: root.model ? root.model.monoChannel : -1

                onActivated: root.model.changeMonoChannel(index)
            }


            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ChannelComboBox {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    channels: root.channels
                    channel: root.model ? root.model.stereoLeftChannel : -1

                    onActivated: root.model.changeStereoLeftChannel(index)
                }

                ChannelComboBox {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    channels: root.channels
                    channel: root.model ? root.model.stereoRightChannel : -1

                    onActivated: root.model.changeStereoRightChannel(index)
                }
            }
        }

        Button {
            Layout.preferredWidth: 38

            text: "X"
            font.bold: true

            onClicked: root.model.remove()
        }
    }
}
