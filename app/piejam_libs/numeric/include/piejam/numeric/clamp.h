// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <type_traits>

namespace piejam::numeric
{

namespace detail
{

struct clamp_fn
{
    template <class T>
        requires(std::is_arithmetic_v<T>)
    [[nodiscard]]
    constexpr auto operator()(T v, T lo, T hi) const noexcept -> T
    {
        if (v < lo)
        {
            v = lo;
        }

        if (v > hi)
        {
            v = hi;
        }

        return v;
    }

    template <class T>
        requires(std::is_arithmetic_v<T>)
    [[nodiscard]]
    constexpr auto operator()(T lo, T hi) const noexcept
    {
        return [=](T v) { return clamp_fn{}(v, lo, hi); };
    }
};

} // namespace detail

inline constexpr detail::clamp_fn clamp{};

} // namespace piejam::numeric
