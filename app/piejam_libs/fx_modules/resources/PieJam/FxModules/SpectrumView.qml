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
import PieJam.Models 1.0 as PJModels
import PieJam.FxModules.Models 1.0 as PJFxModels

SubscribableItem {
    id: root

    property PJFxModels.FxSpectrum model: null

    property bool active: true

    implicitWidth: 636

    ColumnLayout {
        anchors.fill: parent

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            SpectrumGrid {
                anchors.fill: parent
            }

            PJItems.Spectrum {
                anchors.fill: parent

                spectrum: root.model ? root.model.spectrumA : null
                color: Material.color(Material.Pink)
            }

            PJItems.Spectrum {
                anchors.fill: parent

                visible: root.model && root.model.busType === PJModels.Types.BusType.Stereo
                spectrum: root.model ? root.model.spectrumB : null
                color: Material.color(Material.Blue)
            }
        }

        StreamSourceSettings {
            model: root.model

            Layout.fillWidth: true
            Layout.preferredHeight: 48
        }
    }

    onActiveChanged: if (!root.active && root.model) root.model.clear()
}
