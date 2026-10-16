// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15

// A ComboBox with elided display text.
Item {
    id: root

    readonly property alias count: comboBox.count
    property int currentIndex: -1
    property alias currentText: comboBox.currentText
    property alias delegate: comboBox.delegate
    property alias displayText: comboBox.displayText
    property alias elideMode: contentTextItem.elide
    property alias font: comboBox.font
    property var model: null
    property alias popup: comboBox.popup
    property alias textHorizontalAlignment: contentTextItem.horizontalAlignment
    property alias textVerticalAlignment: contentTextItem.verticalAlignment
    property alias textRole: comboBox.textRole
    property alias valueRole: comboBox.valueRole

    signal activated(int index)

    implicitWidth: comboBox.implicitWidth
    implicitHeight: comboBox.implicitHeight

    ComboBox {
        id: comboBox

        anchors.fill: parent

        model: root.model
        currentIndex: root.currentIndex

        contentItem: Text {
            id: contentTextItem

            anchors.fill: comboBox

            text: comboBox.displayText
            font: comboBox.font
            color: root.enabled ? root.Material.foreground : root.Material.secondaryTextColor
            verticalAlignment: Text.AlignVCenter
            padding: 6
        }

        indicator: Canvas {
            anchors.right: comboBox.right
            anchors.bottom: comboBox.bottom
            anchors.bottomMargin: 6

            width: 10
            height: 10

            contextType: "2d"

            onPaint: {
                var radius = 2
                context.reset();
                context.moveTo(0, height);
                context.lineTo(width, 0);
                context.lineTo(width, height - radius);
                context.arc(width - radius, height - radius, radius, 0, Math.PI / 2, false)
                context.closePath();
                context.fillStyle = Material.primaryTextColor;
                context.fill();
            }
        }

        onActivated: root.activated(index)
    }
}
