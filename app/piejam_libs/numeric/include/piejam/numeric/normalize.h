// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <boost/assert.hpp>

#include <cmath>
#include <concepts>
#include <limits>

namespace piejam::numeric
{

namespace detail
{

template <class T, T min, T max, std::floating_point N>
struct normalization
{
    static_assert(min < max);
    static_assert((max - min) <= std::numeric_limits<long long>::max());

    static constexpr N range = static_cast<N>(max - min);
    static constexpr N inv_range = N{1} / range;
};

} // namespace detail

template <std::floating_point N, class T>
    requires std::floating_point<T> || std::integral<T>
[[nodiscard]]
constexpr auto
to_normalized(T const value, T const min, T const max) -> N
{
    BOOST_ASSERT(min < max);
    BOOST_ASSERT(min <= value);
    BOOST_ASSERT(value <= max);
    return static_cast<N>(value - min) / static_cast<N>(max - min);
}

template <std::floating_point N, class T, T min, T max>
    requires std::floating_point<T> || std::integral<T>
[[nodiscard]]
constexpr auto
to_normalized(T const value) -> N
{
    BOOST_ASSERT(min <= value);
    BOOST_ASSERT(value <= max);
    return static_cast<N>(value - min) *
           detail::normalization<T, min, max, N>::inv_range;
}

template <std::floating_point N, class T>
    requires std::floating_point<T> || std::integral<T>
[[nodiscard]]
constexpr auto
from_normalized(N const norm_value, T const min, T const max) -> T
{
    BOOST_ASSERT(min < max);
    BOOST_ASSERT(N{0} <= norm_value);
    BOOST_ASSERT(norm_value <= N{1});

    if constexpr (std::integral<T>)
    {
        return static_cast<T>(std::lround(norm_value * (max - min))) + min;
    }
    else
    {
        return static_cast<T>(norm_value * (max - min) + min);
    }
}

template <std::floating_point N, class T, T min, T max>
    requires std::floating_point<T> || std::integral<T>
[[nodiscard]]
constexpr auto
from_normalized(N const norm_value) -> T
{
    BOOST_ASSERT(N{0} <= norm_value);
    BOOST_ASSERT(norm_value <= N{1});

    constexpr auto range = detail::normalization<T, min, max, N>::range;

    if constexpr (std::integral<T>)
    {
        return static_cast<T>(std::lround(norm_value * range)) + min;
    }
    else
    {
        return static_cast<T>(norm_value * range + min);
    }
}

} // namespace piejam::numeric
