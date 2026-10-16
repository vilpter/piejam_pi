// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import PieJam.Controls 1.0
import PieJam.FxChainControls 1.0
import PieJam.Items 1.0 as PJItems
import PieJam.FxModules.Models 1.0 as PJFxModels
import PieJam.Util 1.0

SubscribableItem {
    id: root

    property PJFxModels.FxFilter model: null

    property bool active: true

    implicitWidth: 636

    RowLayout {
        anchors.fill: parent

        EnumButtonGroup {
            Layout.preferredWidth: 64
            Layout.fillHeight: true

            alignment: Qt.Vertical
            spacing: 0

            model: root.model ? root.model.filterType : null
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            SpectrumGrid {
                anchors.fill: parent

                Repeater {
                    model: [100, 80, 60, 40, 20]

                    delegate: Label {
                        id: resLabel

                        x: parent.width - width - 2
                        y: MathExt.mapTo(modelData, 100, 0, 0, parent.height) + 2
                        width: 24

                        font.pointSize: 6

                        text: modelData + "%"
                        horizontalAlignment: Text.AlignRight
                    }
                }
            }

            PJItems.Spectrum {
                anchors.fill: parent

                spectrum: root.model ? root.model.spectrumIn : null
                color: Material.color(Material.Pink)
            }

            PJItems.Spectrum {
                anchors.fill: parent

                spectrum: root.model ? root.model.spectrumOut : null
                color: Material.color(Material.Blue)
            }

            ParameterXYPad {
                anchors.fill: parent

                modelX: root.model ? root.model.cutoff : null
                modelY: root.model ? root.model.resonance : null
            }
        }

        ParameterControl {
            Layout.fillHeight: true

            paramModel: root.model ? root.model.cutoff : null
        }

        ParameterControl {
            Layout.fillHeight: true

            paramModel: root.model ? root.model.resonance : null
        }
    }

    onActiveChanged: if (!root.active && root.model) root.model.clear()
}
