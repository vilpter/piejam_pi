// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import PieJam.Util 1.0

Item {
    id: root

    property real posX: 0
    property real posY: 0

    property real hotspotX: private_.currentHandle ? private_.currentHandle.width / 2 : 0
    property real hotspotY: private_.currentHandle ? private_.currentHandle.height / 2 : 0

    property Item handle: null

    signal moved(real x, real y)

    QtObject {
        id: private_

        property Item currentHandle: defaultHandle

        readonly property real handleX: MathExt.fromNormalized(root.posX, 0, mouseArea.width) - root.hotspotX
        readonly property real handleY: MathExt.fromNormalized(root.posY, 0, mouseArea.height) - root.hotspotY
    }

    Circle {
        id: defaultHandle
        radius: 6
    }

    Binding {
        target: private_.currentHandle
        property: "x"
        value: private_.handleX
    }

    Binding {
        target: private_.currentHandle
        property: "y"
        value: private_.handleY
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        anchors.margins: 0

        preventStealing: true

        function changePos(mouseX, mouseY) {
            var newPosX = MathExt.clamp(mouseX, 0, mouseArea.width)
            var newPosY = MathExt.clamp(mouseY, 0, mouseArea.height)

            root.moved(MathExt.toNormalized(newPosX, 0, mouseArea.width),
                       MathExt.toNormalized(newPosY, 0, mouseArea.height))
        }

        onPressed: {
            mouseArea.changePos(mouse.x, mouse.y)

            mouse.accepted = true
        }

        onPositionChanged: mouseArea.changePos(mouse.x, mouse.y)
    }

    onHandleChanged: {
        if (private_.currentHandle) {
            private_.currentHandle.parent = null
        }

        private_.currentHandle = root.handle || defaultHandle
        private_.currentHandle.parent = root
    }
}
