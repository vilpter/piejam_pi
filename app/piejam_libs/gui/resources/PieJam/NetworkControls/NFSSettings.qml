// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

    property var model: null

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 0

        TabBar {
            id: nfsTabBar
            Layout.fillWidth: true

            TabButton {
                text: qsTr("NFS Server")
            }
            TabButton {
                text: qsTr("NFS Client")
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: nfsTabBar.currentIndex

            NFSServerSettings {
                model: root.model
            }

            NFSClientSettings {
                model: root.model
            }
        }
    }
}
