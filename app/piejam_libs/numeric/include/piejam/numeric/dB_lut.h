// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/numeric/dB_convert.h>

#include <array>

namespace piejam::numeric
{

template <std::floating_point T, T Min_dB, T Max_dB, std::size_t Size>
inline constexpr auto dB_lut = []() -> std::array<T, Size> {
    static_assert(Min_dB < Max_dB, "Min_dB must be less than Max_dB");

    constexpr T dB_range = Max_dB - Min_dB;
    constexpr T step = dB_range / T{Size - 1};

    std::array<T, Size> lut{};
    lut[0] = T{};
    for (std::size_t i = 1; i < Size; ++i)
    {
        lut[i] = from_dB(Min_dB + static_cast<T>(i) * step);
    }
    return lut;
}();

} // namespace piejam::numeric
