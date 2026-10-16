// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import PieJam.Util 1.0

Item {
    id: root

    property real peakLevel: 1
    property real rmsLevel: 1
    property alias gradient: backgroundRect.gradient
    property color fillColor: "#000000"

    implicitWidth: 40
    implicitHeight: 200

    Rectangle {
        id: backgroundRect

        anchors.fill: parent
    }

    Rectangle {
        color: root.fillColor

        opacity: 0.5

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        height: (1 - root.rmsLevel) * parent.height
    }

    Rectangle {
        color: root.fillColor

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        height: (1 - Math.max(root.peakLevel, root.rmsLevel)) * parent.height
    }
}
