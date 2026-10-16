// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15

import PieJam.Controls 1.0
import PieJam.Models 1.0 as PJModels
import PieJam.Util 1.0

SubscribableItem {
    id: root

    property PJModels.FloatParameter model: null

    QtObject {
        id: private_

        readonly property real value: root.model ? MathExt.fromNormalized(root.model.normalizedValue, -1, 1) : 0
    }

    SymmetricBipolarSlider {
        anchors.fill: parent

        value: private_.value

        onMoved: {
            console.assert(root.model)

            root.model.changeNormalizedValue(MathExt.toNormalized(newValue, -1, 1))
            Info.showParameterValue(root.model)
        }
    }

    MidiAssignArea {
        anchors.fill: parent

        model: root.model ? root.model.midi : null
    }
}
