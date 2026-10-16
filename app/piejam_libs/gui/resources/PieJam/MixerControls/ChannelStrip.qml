// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import PieJam.Controls 1.0

Item {
    id: root

    property var model: null

    property bool deletable: true

    implicitWidth: 132
    implicitHeight: 400

    StackLayout {
        anchors.fill: parent

        currentIndex: MixerViewSettings.mode

        ChannelPerformStrip {
            model: root.model ? root.model.perform : null
        }

        ChannelEditStrip {
            model: root.model ? root.model.edit : null

            deletable: root.deletable
        }

        ChannelFxStrip {
            model: root.model ? root.model.fx : null
        }

        ChannelAuxSend {
            model: root.model ? root.model.auxSend : null
        }
    }
}
