// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

    property alias title: dialog.title
    property alias message: messageLabel.text

    signal yesClicked()
    signal noClicked()
    signal canceled()

    Dialog {
        id: dialog

        parent: Overlay.overlay
        anchors.centerIn: Overlay.overlay

        modal: true

        Label {
            id: messageLabel

            textFormat: Text.PlainText
        }

        footer: Item {
            implicitHeight: 64

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 24
                anchors.rightMargin: 24
                anchors.bottomMargin: 24
                anchors.topMargin: 0

                Button {
                    text: qsTr("Cancel")

                    onClicked: {
                        dialog.close()
                        root.canceled();
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    text: qsTr("Yes")

                    onClicked: {
                        dialog.close()
                        root.yesClicked()
                    }
                }

                Button {
                    text: qsTr("No")

                    onClicked: {
                        dialog.close()
                        root.noClicked()
                    }
                }
            }
        }
    }

    function open() {
        dialog.open()
    }
}
