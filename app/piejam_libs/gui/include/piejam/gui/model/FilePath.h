// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

#include <filesystem>

namespace piejam::gui::model
{

struct FilePath
{
    Q_GADGET

public:
    std::filesystem::path path;
};

} // namespace piejam::gui::model

Q_DECLARE_METATYPE(piejam::gui::model::FilePath)
