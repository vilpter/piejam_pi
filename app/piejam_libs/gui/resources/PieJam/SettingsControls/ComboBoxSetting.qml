// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import PieJam.Controls 1.0

Item {
    id: root

    property alias name: nameLabel.text
    property alias model: comboBox.model
    property alias currentIndex: comboBox.currentIndex
    property alias delegate: comboBox.delegate
    property string emptyText: qsTr("Not Available")
    property string unselectedText: qsTr("Select...")
    property var displayText: undefined

    signal optionSelected(int index)

    implicitWidth: 300
    implicitHeight: 96

    Frame {
        id: frame

        anchors.fill: parent

        spacing: 0

        ColumnLayout {
            anchors.fill: parent

            Label {
                id: nameLabel

                Layout.fillWidth: true

                textFormat: Text.PlainText
                font.pixelSize: 18
            }

            ChoiceBox {
                id: comboBox

                Layout.fillWidth: true

                displayText: comboBox.count === 0
                        ? root.emptyText
                        : (comboBox.currentIndex === -1
                                ? root.unselectedText
                                : (root.displayText !== undefined ? root.displayText : comboBox.currentText))
                enabled: comboBox.count !== 0

                onActivated: root.optionSelected(index)

                textRole: "text"
            }
        }
    }
}
