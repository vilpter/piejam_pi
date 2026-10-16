// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cmath>
#include <concepts>
#include <limits>

namespace piejam::numeric
{

template <std::floating_point T>
[[nodiscard]]
constexpr auto
almost_equal(T a, T b) noexcept -> bool
{
    constexpr T abs_eps =
        std::numeric_limits<T>::epsilon() * 100; // near-zero tolerance
    constexpr T rel_eps = static_cast<T>(1e-5);  // relative tolerance

    T const diff = std::fabs(a - b);
    if (diff <= abs_eps)
    {
        return true;
    }

    T const largest = std::max(std::fabs(a), std::fabs(b));
    return diff <= largest * rel_eps;
}

template <std::floating_point T>
constexpr auto
exact_equal(T a, T b) noexcept -> bool
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    return a == b;
#pragma GCC diagnostic pop
}

} // namespace piejam::numeric
