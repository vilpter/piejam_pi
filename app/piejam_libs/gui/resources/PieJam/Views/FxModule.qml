// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import QtQml 2.15

import PieJam.Controls 1.0
import PieJam.Models 1.0 as PJModels

ViewPane {
    id: root

    QtObject {
        id: private_

        readonly property bool active: root.model && root.model.active ? root.model.active.value : false
        readonly property var content: root.model ? root.model.content : null
        readonly property var contentType: private_.content ? private_.content.type : null
    }

    Material.primary: root.model ? root.model.color : Material.Pink
    Material.accent: root.model ? root.model.color : Material.Pink

    RowLayout {
        anchors.fill: parent

        spacing: 0

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 8

            HeaderLabel {
                Layout.fillWidth: true
                Layout.preferredHeight: 24

                text: root.model ? (root.model.chainName + " - " + root.model.name) : ""
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true

                currentIndex: private_.contentType ? PJModels.FxModuleRegistry.indexOf(private_.contentType) : -1

                Repeater {
                    model: PJModels.FxModuleRegistry.items

                    delegate: BusyLoader {
                        id: fxModuleViewLoader

                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        source: modelData.viewSource

                        Binding {
                            target: fxModuleViewLoader.item
                            property: "model"
                            value: private_.contentType === modelData.fxType ? private_.content : null
                        }

                        Binding {
                            target: fxModuleViewLoader.item
                            property: "active"
                            value: private_.active
                        }
                    }
                }
            }
        }

        ViewToolBar {
            Layout.fillHeight: true

            ViewToolBarParameterButton {
                iconSource: "qrc:///images/icons/power.svg"
                model: root.model ? root.model.active : null
            }

            ViewToolBarButton {
                iconSource: "qrc:///images/icons/chevron-up.svg"
                checkable: false
                enabled: root.model && root.model.canShowPrev

                onClicked: root.model.showPrevModule()
            }

            ViewToolBarButton {
                iconSource: "qrc:///images/icons/chevron-down.svg"
                checkable: false
                enabled: root.model && root.model.canShowNext

                onClicked: root.model.showNextModule()
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }
}
