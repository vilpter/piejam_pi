// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>

namespace piejam::numeric
{

namespace detail
{

template <std::size_t N>
struct pow_n_fn
{
    template <class T>
    constexpr auto operator()(T const t) const noexcept
    {
        if constexpr (N == 0)
        {
            return T{1};
        }
        else if constexpr (N == 1)
        {
            return t;
        }
        else if constexpr (N % 2 == 0)
        {
            auto half = pow_n_fn<N / 2>{}(t);
            return half * half;
        }
        else
        {
            auto half = pow_n_fn<N / 2>{}(t);
            return half * half * t;
        }
    }
};

} // namespace detail

template <std::size_t N>
inline constexpr detail::pow_n_fn<N> pow_n{};

} // namespace piejam::numeric
