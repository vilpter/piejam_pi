// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import PieJam.Models 1.0 as PJModels

SubscribableItem {
    id: root

    property PJModels.String model: null

    implicitWidth: item.implicitWidth
    implicitHeight: item.implicitHeight

    TextField {
        id: item

        anchors.fill: parent

        text: root.model ? root.model.value : ""

        onEditingFinished: {
            root.model.changeValue(text)
            focus = false
        }
    }
}
