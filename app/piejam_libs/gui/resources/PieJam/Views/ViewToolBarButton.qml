// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

    property alias iconSource: button.icon.source
    property alias text: button.text
    property bool checkable: true
    property bool checked: false

    signal clicked()

    implicitWidth: 32
    implicitHeight: 40

    Layout.alignment: Qt.AlignHCenter

    Button {
        id: button

        anchors.fill: parent

        icon.width: 24
        icon.height: 24

        font.pixelSize: 12

        checkable: root.checkable
        checked: root.checked

        onClicked: root.clicked()
    }
}
