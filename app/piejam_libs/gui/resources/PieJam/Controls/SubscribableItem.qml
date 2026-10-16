// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import QtQml 2.15

Item {
    id: root

    enabled: root.model !== null

    ModelSubscription {
        target: root.model
        subscribed: root.visible
    }
}
