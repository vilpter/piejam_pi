// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15

import PieJam.Controls 1.0

ChoiceBox {
    id: root

    property var channels: ["-"]
    property int channel: -1

    model: root.channels
    currentIndex: channel
    textHorizontalAlignment: Text.AlignHCenter

    textRole: "text"
    displayText: channel > 0 ? channel : "-"

    Material.foreground: channel >= root.channels.count ? Material.Red : Material.primaryTextColor
}
