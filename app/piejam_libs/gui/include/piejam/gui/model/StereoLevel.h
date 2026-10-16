// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>

#include <piejam/audio/pair.h>

#include <QObject>

#include <boost/assert.hpp>

namespace piejam::gui::model
{

class StereoLevel : public QObject
{
    Q_OBJECT

    PIEJAM_GUI_PROPERTY(double, levelLeft, setLevelLeft)
    PIEJAM_GUI_PROPERTY(double, levelRight, setLevelRight)

public:
    template <audio::pair_channel C>
    double level() const
    {
        return C == audio::pair_channel::left ? levelLeft() : levelRight();
    }

    template <audio::pair_channel C>
    void setLevel(double level)
    {
        C == audio::pair_channel::left ? setLevelLeft(level)
                                       : setLevelRight(level);
    }

    void reset()
    {
        setLevelLeft(0.);
        setLevelRight(0.);
    }
};

} // namespace piejam::gui::model
