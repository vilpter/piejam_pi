// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import PieJam.Controls 1.0
import PieJam.Models 1.0 as PJModels

ChannelStripBase {
    id: root

    property bool deletable: true

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        TextField {
            id: nameText

            Layout.fillWidth: true

            text: root.model ? root.model.name : ""

            placeholderText: qsTr("Name")

            onEditingFinished: {
                root.model.changeName(nameText.text)
                focus = false
            }
        }

        ColorSelector {
            Layout.preferredWidth: 40
            Layout.preferredHeight: 52
            Layout.alignment: Qt.AlignHCenter

            selectedColor: root.model ? root.model.color : Material.Pink

            onColorSelected: root.model.changeColor(newSelectedColor)
        }

        Label {
            Layout.fillWidth: true

            textFormat: Text.PlainText
            text: qsTr("Audio In")
        }

        AudioRoutingComboBox {
            Layout.fillWidth: true

            enabled: root.model && root.model.channelType !== PJModels.Types.ChannelType.Aux

            model: root.model ? root.model.in : null
        }

        Label {
            Layout.fillWidth: true

            textFormat: Text.PlainText
            text: qsTr("Audio Out")
        }

        AudioRoutingComboBox {
            Layout.fillWidth: true

            model: root.model ? root.model.out : null
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        RowLayout {
            id: moveButtonsRow

            Layout.fillWidth: true

            visible: root.deletable
            enabled: root.deletable

            spacing: 8

            Button {
                Layout.fillWidth: true

                enabled: root.model && root.model.canMoveLeft

                icon.width: 24
                icon.height: 24
                icon.source: "qrc:///images/icons/chevron-left.svg"

                onClicked: root.model.moveLeft()
            }

            Button {
                Layout.fillWidth: true

                enabled: root.model && root.model.canMoveRight

                icon.width: 24
                icon.height: 24
                icon.source: "qrc:///images/icons/chevron-right.svg"

                onClicked: root.model.moveRight()
            }
        }

        Button {
            visible: root.deletable
            enabled: root.deletable

            Layout.fillWidth: true

            text: qsTr("Delete")
            font.bold: true

            Material.background: Material.color(Material.Red, Material.Shade400)

            onClicked: root.model.deleteChannel()
        }
    }
}
