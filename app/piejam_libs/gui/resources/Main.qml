// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Window 2.15

import QtQml 2.15

import PieJam.Dialogs 1.0
import PieJam.Models 1.0 as PJModels
import PieJam.Views 1.0

Window {
    id: root

    property PJModels.Root model: null

    width: 800
    height: 480

    QtObject {
        id: private_

        readonly property var contentOrientation: [
            Qt.PortraitOrientation,
            Qt.LandscapeOrientation,
            Qt.InvertedPortraitOrientation,
            Qt.InvertedLandscapeOrientation
        ]

        readonly property int rotationAngle: root.model ? (360 - root.model.displayRotation) % 360 : 0
    }

    contentOrientation: private_.contentOrientation[private_.rotationAngle / 90]

    Item {
        width: (private_.rotationAngle === 0 || private_.rotationAngle === 180) ? parent.width : parent.height
        height: (private_.rotationAngle === 0 || private_.rotationAngle === 180) ? parent.height : parent.width
        x: (private_.rotationAngle === 90 || private_.rotationAngle === 180) ? parent.width : 0
        y: (private_.rotationAngle === 180 || private_.rotationAngle === 270) ? parent.height : 0

        Binding on transform {
            when: private_.rotationAngle !== 0
            value: Rotation {
                origin.x: 0
                origin.y: 0
                angle: private_.rotationAngle
            }
            restoreMode: Binding.RestoreBindingOrValue
        }

        RootView {
            anchors.fill: parent

            model: root.model

            Component.onCompleted: root.visible = true
        }
    }
}
