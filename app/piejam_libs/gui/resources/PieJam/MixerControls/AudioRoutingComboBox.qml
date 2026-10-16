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

    property PJModels.AudioRouting model: null

    property bool hasSelectableDefault: true

    implicitHeight: comboBox.implicitHeight

    ChoiceBox {
        id: comboBox

        anchors.fill: parent

        displayText: root.model ? root.model.selected.label : "-"
        elideMode: Qt.ElideMiddle

        function selectedStateToColor(s) {
            switch (s) {
                case PJModels.AudioRoutingSelection.State.Invalid:
                    return Material.Red

                case PJModels.AudioRoutingSelection.State.NotMixed:
                    return Material.Yellow

                default:
                    return Material.primaryTextColor
            }
        }

        Material.foreground: selectedStateToColor(root.model ? root.model.selected.state : null)

        popup: Menu {
            cascade: true

            MenuItem {
                enabled: root.hasSelectableDefault

                visible: root.hasSelectableDefault
                height: visible ? implicitHeight : 0

                text: "None"

                onClicked: root.model.changeToNone()
            }

            MenuItem {
                enabled: root.model && root.model.mixIsValid

                visible: root.model && root.model.mixIsAvailable
                height: visible ? implicitHeight : 0

                text: "Mix"

                onClicked: root.model.changeToMix()
            }

            Menu {
                title: qsTr("Devices")

                enabled: devicesRep.count > 0

                Repeater {
                    id: devicesRep

                    model: root.model ? root.model.devices : null

                    delegate: MenuItem {
                        text: model.item.value

                        onClicked: root.model.changeToDevice(index)

                        ModelSubscription {
                            target: model.item
                            subscribed: parent.visible
                        }
                    }
                }
            }

            Menu {
                title: qsTr("Channels")

                enabled: channelsRep.count > 0

                Repeater {
                    id: channelsRep

                    model: root.model ? root.model.channels : null

                    delegate: MenuItem {
                        text: model.item.value

                        onClicked: root.model.changeToChannel(index)

                        ModelSubscription {
                            target: model.item
                            subscribed: parent.visible
                        }
                    }
                }
            }
        }
    }
}
