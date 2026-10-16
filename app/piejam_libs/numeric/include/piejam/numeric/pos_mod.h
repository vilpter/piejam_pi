// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <boost/assert.hpp>

#include <concepts>

namespace piejam::numeric
{

template <std::integral T>
[[nodiscard]]
constexpr auto
pos_mod(T x, T y) noexcept -> T
{
    BOOST_ASSERT(y != 0);

    T const n = x % y;

    if constexpr (std::signed_integral<T>)
    {
        return n < 0 ? n + (y > 0 ? y : -y) : n;
    }
    else
    {
        return n;
    }
}

} // namespace piejam::numeric
