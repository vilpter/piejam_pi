// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

Item {
    id: root

    property int radius: 50
    property color color: "white"

    property real centerX: x + radius
    property real centerY: y + radius

    onCenterXChanged: x = centerX - radius
    onCenterYChanged: y = centerY - radius
    onXChanged: centerX = x + radius
    onYChanged: centerY = y + radius

    width: radius * 2
    height: radius * 2

    Rectangle {
        anchors.fill: parent
        color: root.color
        radius: root.radius
    }
}
