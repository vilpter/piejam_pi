// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import PieJam.Controls 1.0
import PieJam.Models 1.0 as PJModels

Item {
    id: root

    property PJModels.FloatParameter modelX: null
    property PJModels.FloatParameter modelY: null

    QtObject {
        id: private_

        readonly property real valueX: root.modelX ? root.modelX.normalizedValue : 0
        readonly property real valueY: root.modelY ? 1 - root.modelY.normalizedValue : 0
    }

    XYPad {
        anchors.fill: parent

        posX: private_.valueX
        posY: private_.valueY

        handle: Circle {
            radius: 6
            color: Material.color(Material.Yellow, Material.Shade400)
        }

        onMoved: {
            if (root.modelX)
                root.modelX.changeNormalizedValue(x)

            if (root.modelY)
                root.modelY.changeNormalizedValue(1 - y)
        }
    }

    ModelSubscription {
        target: root.modelX
        subscribed: root.visible
    }

    ModelSubscription {
        target: root.modelY
        subscribed: root.visible
    }
}
