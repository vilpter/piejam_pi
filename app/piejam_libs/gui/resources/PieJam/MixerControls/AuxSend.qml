// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import PieJam.Controls 1.0
import PieJam.Models 1.0 as PJModels
import PieJam.ParameterControls 1.0

SubscribableItem {
    id: root

    property PJModels.AuxSend model: null

    property int expandedHeight: 362

    signal expanded()

    implicitHeight: private_.expanded ? root.expandedHeight : 72

    QtObject {
        id: private_

        property bool expanded: false

        onExpandedChanged: {
            if (expanded)
                root.expanded()
        }
    }

    Rectangle {
        anchors.fill: parent

        border.color: root.model && root.model.active && root.model.active.value ? Material.primaryColor : Material.frameColor
        border.width: 2
        color: Material.backgroundColor
        radius: 4

        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: 4
            anchors.rightMargin: 4

            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 40

                Label {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40

                    leftPadding: 8
                    verticalAlignment: Text.AlignVCenter

                    text: root.model ? root.model.name : "name"
                    font.bold:  true
                    color: root.model && root.model.enabled ? Material.primaryTextColor : Material.secondaryTextColor
                }

                ParameterToggleButton {
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 40

                    model: root.model ? root.model.active : null

                    icon.source: (!root.model || root.model.canToggle)
                                 ? "qrc:///images/icons/power.svg"
                                 : "qrc:///images/icons/cycle_arrows.svg"

                    icon.color: root.model && root.model.active && root.model.active.value
                            ? Material.primaryColor
                            : (root.model && root.model.canToggle ? Material.primaryTextColor : Material.secondaryTextColor)

                    flat: true
                    enabled: root.model && root.model.canToggle
                }
            }

            ValueLabel {
                Layout.fillWidth: true
                Layout.preferredHeight: 32

                model: root.model ? root.model.volume : null

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                color: root.model && root.model.enabled ? Material.primaryTextColor : Material.secondaryTextColor

                MouseArea {
                    anchors.fill: parent

                    onClicked: private_.expanded = !private_.expanded
                }
            }

            VolumeFader {
                Layout.fillWidth: true
                Layout.fillHeight: true

                visible: private_.expanded

                model: root.model ? root.model.volume : null
                muted: !root.model || !root.model.enabled

                scaleData: PJModels.MixerDbScales.sendFaderScale
            }

            EnumComboBox {
                Layout.fillWidth: true
                Layout.preferredHeight: 40

                visible: private_.expanded

                model: root.model ? root.model.faderTap : null
            }
        }
    }
}
