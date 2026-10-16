// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>

namespace piejam::gui::model
{

class SoundCardInfo
{
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(int numIns MEMBER numIns)
    Q_PROPERTY(int numOuts MEMBER numOuts)
public:
    QString name{};
    int numIns{};
    int numOuts{};

    constexpr auto operator==(SoundCardInfo const&) const noexcept
        -> bool = default;
};

} // namespace piejam::gui::model

Q_DECLARE_METATYPE(piejam::gui::model::SoundCardInfo)
