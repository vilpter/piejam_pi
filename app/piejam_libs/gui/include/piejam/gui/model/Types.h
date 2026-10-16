// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <qobjectdefs.h>

#include <piejam/audio/types.h>

namespace piejam::gui::model
{

Q_NAMESPACE

enum class BusType : bool
{
    Mono,
    Stereo,
};

Q_ENUM_NS(BusType)

enum class ChannelType
{
    Mono,
    Stereo,
    Aux,
};

Q_ENUM_NS(ChannelType)

enum class DFTResolution
{
    Low,
    Medium,
    High,
    VeryHigh,
};

Q_ENUM_NS(DFTResolution)

enum class StereoChannel
{
    Left,
    Right,
    Middle,
    Side
};

Q_ENUM_NS(StereoChannel)

enum class TriggerSlope
{
    RisingEdge,
    FallingEdge
};

Q_ENUM_NS(TriggerSlope)

} // namespace piejam::gui::model
