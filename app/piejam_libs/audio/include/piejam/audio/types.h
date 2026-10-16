// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/enum.h>

#include <cstddef>

namespace piejam::audio
{

enum class bus_type : bool
{
    mono,
    stereo
};

[[nodiscard]]
constexpr auto
num_channels(bus_type const b) -> std::size_t
{
    return bool_enum_to(b, 1uz, 2uz);
}

enum class bus_channel
{
    mono,
    left,
    right
};

} // namespace piejam::audio
