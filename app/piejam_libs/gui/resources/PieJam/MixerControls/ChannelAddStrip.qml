// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import PieJam.Models 1.0 as PJModels

ChannelStripBase {
    id: root

    property PJModels.MixerChannelAdd model: null

    signal channelAdded()

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        TextField {
            id: nameText

            Layout.fillWidth: true

            placeholderText: qsTr("<New>")
            placeholderTextColor: Material.secondaryTextColor

            onEditingFinished: focus = false
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Button {
            Layout.fillWidth: true

            text: qsTr("+Mono")
            font.bold: true

            Material.background: Material.color(Material.Green, Material.Shade400)

            onClicked: {
                root.model.addMonoChannel(nameText.text)
                nameText.text = ""
                root.channelAdded()
            }
        }

        Button {
            Layout.fillWidth: true

            text: qsTr("+Stereo")
            font.bold: true

            Material.background: Material.color(Material.Green, Material.Shade400)

            onClicked: {
                root.model.addStereoChannel(nameText.text)
                nameText.text = ""
                root.channelAdded()
            }
        }

        Button {
            Layout.fillWidth: true

            text: qsTr("+Aux")
            font.bold: true

            Material.background: Material.color(Material.Green, Material.Shade400)

            onClicked: {
                root.model.addAuxChannel(nameText.text)
                nameText.text = ""
                root.channelAdded()
            }
        }
    }
}
