// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import PieJam.Controls 1.0
import PieJam.Models 1.0 as PJModels
import PieJam.Util 1.0

SubscribableItem {
    id: root

    property PJModels.FloatParameter model: null

    QtObject {
        id: private_

        readonly property var paramModel: root.model
        readonly property real value: private_.paramModel ? private_.paramModel.normalizedValue : 0
        readonly property bool bipolar: private_.paramModel ? private_.paramModel.bipolar : false
    }

    ColumnLayout {
        anchors.fill: parent

        spacing: 0

        RowLayout {
            spacing: 2

            ValueIndicatorBar {
                Layout.preferredWidth: 0.10 * parent.width
                Layout.fillHeight: true
                Layout.leftMargin: 2
                Layout.topMargin: 2
                Layout.bottomMargin: 2

                value: private_.value
                bipolar: private_.bipolar
                orientation: Qt.Vertical
                direction: Qt.RightToLeft
            }

            Touchstrip {
                value: private_.value

                resolution: height * (resolutionButton.checked ? 4 : 1)

                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.rightMargin: 2
                Layout.topMargin: 2
                Layout.bottomMargin: 2

                onChangeValue: {
                    if (private_.paramModel) {
                        private_.paramModel.changeNormalizedValue(newValue)
                        Info.showParameterValue(private_.paramModel)
                    }
                }
            }

            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Button {
            id: resolutionButton

            checkable: true
            display: AbstractButton.IconOnly
            icon.source: "qrc:///images/icons/arrow-expand-vertical.svg"

            Layout.leftMargin: 2
            Layout.rightMargin: 2
            Layout.fillWidth: true
            Layout.maximumHeight: 40
        }
    }

    MidiAssignArea {
        anchors.fill: parent

        model: private_.paramModel ? private_.paramModel.midi : null
    }
}
