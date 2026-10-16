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

    property PJModels.EnumParameter model: null

    property var icons: null
    property int alignment: Qt.Horizontal
    property int spacing: 5

    GridLayout {
        anchors.fill: parent

        columns: root.alignment === Qt.Vertical ? 1 : Number.MAX_VALUE
        rows: root.alignment === Qt.Horizontal ? 1 : Number.MAX_VALUE
        columnSpacing: root.spacing
        rowSpacing: root.spacing

        Repeater {
            model: root.model ? root.model.values : null

            delegate: Button {
                Layout.fillWidth: true
                Layout.fillHeight: true

                text: model.text

                checkable: true
                autoExclusive: true

                icon.source: root.icons ? root.icons[model.index] : ""

                display: icons ? AbstractButton.IconOnly : AbstractButton.TextOnly

                checked: model.value === root.model.value

                onClicked: {
                    root.model.changeValue(model.value)
                    Info.showParameterValue(root.model)
                }
            }
        }
    }

    MidiAssignArea {
        anchors.fill: parent

        model: root.model ? root.model.midi : null
    }
}
