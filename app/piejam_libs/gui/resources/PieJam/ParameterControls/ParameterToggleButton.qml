// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15

import PieJam.Controls 1.0
import PieJam.Models 1.0 as PJModels

SubscribableItem {
    id: root

    property PJModels.BoolParameter model: null

    property bool flat: false
    property alias text: button.text
    property alias icon: button.icon

    QtObject {
        id: private_

        readonly property var paramModel: root.model
        readonly property bool value: private_.paramModel && private_.paramModel.value
    }

    Button {
        id: button

        anchors.fill: parent

        flat: root.flat
        checkable: !root.flat
        checked: !root.flat && private_.value
        highlighted: root.flat && private_.value

        onClicked: {
            if (private_.paramModel) {
                private_.paramModel.changeValue(!private_.value)
                Info.showParameterValue(private_.paramModel)
            }
        }
    }

    MidiAssignArea {
        anchors.fill: parent

        model: private_.paramModel ? private_.paramModel.midi : null
    }
}
