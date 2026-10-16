// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: root

    property string ssid: ""
    property string securityType: ""
    property alias password: passwordField.text
    property alias rememberNetwork: rememberCheckBox.checked

    parent: Overlay.overlay
    x: Math.round((parent.width - width) / 2)
    y: 8
    width: Math.min(550, parent.width - 16)
    topPadding: 4
    bottomPadding: 4

    modal: true
    focus: true

    standardButtons: Dialog.Cancel | Dialog.Ok

    onOpened: {
        passwordField.text = ""
        passwordField.forceActiveFocus()
    }

    onAccepted: {
        // Will be handled by caller
    }

    onRejected: {
        passwordField.text = ""
    }

    // Title row with Remember checkbox inline
    header: RowLayout {
        spacing: 8

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.topMargin: 8
            Layout.bottomMargin: 4
            text: qsTr("Connect to ") + root.ssid
            font.pixelSize: 16
            font.bold: true
            elide: Text.ElideRight
        }

        CheckBox {
            id: rememberCheckBox

            Layout.rightMargin: 8
            text: qsTr("Remember")
            checked: true
            font.pixelSize: 12
        }
    }

    ColumnLayout {
        width: parent.width
        spacing: 0

        // Password row: Security label + password field
        RowLayout {
            Layout.fillWidth: true
            visible: root.securityType !== "Open"
            spacing: 8

            Label {
                text: root.securityType + " Password"
                font.pixelSize: 12
                opacity: 0.7
                visible: root.securityType !== "" && root.securityType !== "Open"
            }

            TextField {
                id: passwordField

                Layout.fillWidth: true

                placeholderText: qsTr("Enter password")
                echoMode: TextInput.Normal
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText

                Keys.onReturnPressed: root.accept()
                Keys.onEnterPressed: root.accept()
            }
        }

        // Info text for open networks
        Label {
            Layout.fillWidth: true
            visible: root.securityType === "Open"
            text: qsTr("This is an open network. No password required.")
            font.italic: true
            opacity: 0.7
            wrapMode: Text.WordWrap
        }
    }
}
