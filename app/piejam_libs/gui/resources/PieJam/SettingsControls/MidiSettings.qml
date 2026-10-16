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

    property PJModels.MidiInputSettings model: null

    TabBar {
        id: tabs

        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: parent.top

        currentIndex: 0

        TabButton {
            text: "Input"
        }
    }

    StackLayout {
        id: stackView

        anchors.top: tabs.bottom
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.left: parent.left

        currentIndex: tabs.currentIndex

        ListView {
            id: midiDevicesList

            orientation: ListView.Vertical

            model: root.model.devices

            delegate: MidiDeviceConfig {
                name: model.item.name
                deviceEnabled: model.item.enabled

                anchors.left: parent ? parent.left : undefined
                anchors.right: parent ? parent.right : undefined

                onEnabledToggled: model.item.changeEnabled(newEnabled)

                ModelSubscription {
                    target: model.item
                    subscribed: visible
                }
            }
        }
    }
}
