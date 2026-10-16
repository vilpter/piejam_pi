// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/model/FilePath.h>

#include <QObject>

namespace piejam::gui::model
{

class Filesystem : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE bool fileExists(piejam::gui::model::FilePath const&) const;
    Q_INVOKABLE bool isRegularFile(piejam::gui::model::FilePath const&) const;
    Q_INVOKABLE bool isDirectory(piejam::gui::model::FilePath const&) const;
};

auto filesystemSingleton() -> Filesystem&;

} // namespace piejam::gui::model
