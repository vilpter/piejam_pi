// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import PieJam.Controls 1.0
import PieJam.Models 1.0 as PJModels
import PieJam.ParameterControls 1.0

ChannelStripBase {
    id: root

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        HeaderLabel {
            Layout.fillWidth: true
            Layout.preferredHeight: 24

            text: root.model ? root.model.name : ""
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            currentIndex: root.model && root.model.aux ? 0 : 1

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true

                spacing: 4

                Label {
                    Layout.fillWidth: true

                    textFormat: Text.PlainText
                    text: qsTr("Fader Tap")
                }

                EnumComboBox {
                    Layout.fillWidth: true

                    model: root.model && root.model.aux ? root.model.aux.defaultFaderTap : null
                }
            }

            ListView {
                id: auxSends

                Layout.fillWidth: true
                Layout.fillHeight: true

                spacing: 2

                clip: true
                boundsBehavior: Flickable.StopAtBounds
                boundsMovement: Flickable.StopAtBounds

                model: root.model ? root.model.sends : null

                delegate: AuxSend {
                    anchors.left: parent ? parent.left : undefined
                    anchors.right: parent ? parent.right : undefined

                    expandedHeight: auxSends.height

                    model: item

                    onExpanded: auxSends.slideToIndex(index)
                }

                Behavior on contentY {
                    NumberAnimation { duration: 400; easing.type: Easing.InOutQuad }
                }

                function slideToIndex(idx) {
                    // Force delegate creation & correct geometry
                    auxSends.positionViewAtIndex(idx, ListView.Contain);

                    // Animate to its Y position
                    var item = itemAtIndex(idx);
                    if (item) {
                        contentY = item.y;
                    }
                }
            }
        }
    }
}
