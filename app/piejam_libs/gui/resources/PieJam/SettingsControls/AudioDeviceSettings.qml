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

    property PJModels.AudioDeviceSettings model: null

    implicitWidth: 752
    implicitHeight: 432

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        ComboBoxSetting {
            Layout.fillWidth: true

            model: root.model.soundCards
            currentIndex: root.model.selectedSoundCardIndex

            delegate: ItemDelegate {
                id: delegate

                implicitHeight: 48
                width: parent ? parent.width : undefined

                contentItem: Item {
                    anchors.fill: parent
                    anchors.margins: 4

                    ColumnLayout {
                        anchors.fill: parent

                        Text {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            color: delegate.highlighted ? Material.accent : Material.foreground
                            elide: Text.ElideRight
                            font.pixelSize: 16
                            verticalAlignment: Text.AlignVCenter
                            text: model.value.name
                        }

                        Text {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            text: "Ins: " + model.value.numIns + " Outs: " + model.value.numOuts
                            font.pixelSize: 14
                            color: Material.hintTextColor
                        }
                    }
                }

                highlighted: root.model.selectedSoundCardIndex === index
            }

            name: qsTr("Device")

            onOptionSelected: root.model.selectSoundCard(index)
        }

        ComboBoxSetting {
            Layout.fillWidth: true

            visible: root.model.selectedSoundCardIndex !== -1

            model: root.model.sampleRates
            currentIndex: root.model.selectedSampleRate

            name: qsTr("Sample rate")

            onOptionSelected: root.model.selectSampleRate(index)
        }

        Frame {
            Layout.fillWidth: true
            Layout.preferredHeight: 112

            spacing: 0

            visible: root.model.selectedSoundCardIndex !== -1

            ColumnLayout {
                anchors.fill: parent

                Label {
                    Layout.fillWidth: true

                    textFormat: Text.PlainText
                    font.pixelSize: 18

                    text: qsTr("Buffer size")
                }

                Label {
                    Layout.fillWidth: true

                    textFormat: Text.PlainText
                    font.pixelSize: 14

                    text: (root.model.selectedPeriodSizeIndex === -1 ? "0" : root.model.selectedPeriodSize) +
                            " Samples / " +
                            root.model.bufferLatency.toFixed(2) +
                            " ms"
                }

                Slider {
                    Layout.fillWidth: true

                    from: 0
                    to: root.model.periodSizesCount
                    stepSize: 1
                    value: root.model.selectedPeriodSizeIndex

                    onMoved: root.model.selectPeriodSize(value)
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
