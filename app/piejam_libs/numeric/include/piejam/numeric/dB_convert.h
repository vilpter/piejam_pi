// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/numeric/constants.h>

#include <boost/assert.hpp>

#include <cmath>
#include <concepts>
#include <limits>

namespace piejam::numeric
{

template <std::floating_point T>
[[nodiscard]]
constexpr auto
to_dB(T lin) noexcept -> T
{
    static_assert(std::numeric_limits<T>::is_iec559, "IEEE 754 required");
    BOOST_ASSERT(lin > T{0});
    return std::log10(lin) * T{20};
}

template <std::floating_point T>
[[nodiscard]]
constexpr auto
to_dB(T lin, T min) noexcept -> T
{
    return lin <= min ? constants::negative_inf<T> : to_dB(lin);
}

template <std::floating_point T>
[[nodiscard]]
constexpr auto
from_dB(T dB) noexcept -> T
{
    BOOST_ASSERT(std::isfinite(dB));
    return std::pow(T{10}, dB / T{20});
}

template <std::floating_point T>
[[nodiscard]]
constexpr auto
from_dB(T dB, T min_dB) noexcept -> T
{
    return dB <= min_dB ? T{} : from_dB(dB);
}

} // namespace piejam::numeric
