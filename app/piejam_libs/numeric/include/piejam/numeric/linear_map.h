// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <boost/assert.hpp>

#include <concepts>

namespace piejam::numeric
{

struct linear_map_fn
{
    template <std::floating_point T>
    [[nodiscard]]
    constexpr auto
    operator()(T v, T src_lo, T src_hi, T dst_lo, T dst_hi) const noexcept -> T
    {
        BOOST_ASSERT(src_lo != src_hi);
        return ((v - src_lo) / (src_hi - src_lo)) * (dst_hi - dst_lo) + dst_lo;
    }

    template <std::floating_point T>
    [[nodiscard]]
    constexpr auto
    operator()(T src_lo, T src_hi, T dst_lo, T dst_hi) const noexcept
    {
        BOOST_ASSERT(src_lo != src_hi);

        T const scale = (dst_hi - dst_lo) / (src_hi - src_lo);
        return [=](T v) noexcept { return (v - src_lo) * scale + dst_lo; };
    }
};

inline constexpr linear_map_fn linear_map{};

} // namespace piejam::numeric
