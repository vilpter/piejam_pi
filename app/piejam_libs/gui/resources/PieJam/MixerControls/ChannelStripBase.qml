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

    property PJModels.MixerChannel model: null

    default property alias contentData: frame.contentData

    implicitWidth: 132

    Material.primary: root.model ? root.model.color : Material.Pink
    Material.accent: root.model ? root.model.color : Material.Pink

    Frame {
        id: frame

        anchors.fill: parent
    }
}
