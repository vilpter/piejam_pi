// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import PieJam.Controls 1.0
import PieJam.Models 1.0 as PJModels

SubscribableItem {
    id: root

    property PJModels.EnumParameter model: null

    implicitWidth: comboBox.implicitWidth
    implicitHeight: comboBox.implicitHeight

    ChoiceBox {
        id: comboBox

        anchors.fill: parent;

        model: root.model ? root.model.values : null
        currentIndex: root.model ? root.model.value - root.model.minValue : -1
        textRole: "text"
        valueRole: "value"

        onActivated: {
            if (root.model) {
                root.model.changeValue(root.model.minValue + index)
                Info.showParameterValue(root.model)
            }
        }
    }

    MidiAssignArea {
        anchors.fill: parent

        model: root.model ? root.model.midi : null
    }
}
