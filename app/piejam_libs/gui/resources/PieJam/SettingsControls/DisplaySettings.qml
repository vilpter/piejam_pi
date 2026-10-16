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

        ComboBoxSetting {
            Layout.fillWidth: true

            name: qsTr("Rotation")
            model: root.model ? root.model.rotations : null
            currentIndex: root.model.rotation

            onOptionSelected: root.model.selectRotation(index)
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
