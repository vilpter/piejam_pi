// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/numeric/pow_n.h>

#include <cmath>
#include <concepts>
#include <numeric>
#include <ranges>

namespace piejam::numeric
{

namespace detail
{

struct rms_fn
{
    template <std::ranges::input_range R>
        requires std::floating_point<std::ranges::range_value_t<R>>
    constexpr auto operator()(R const& in) const noexcept
    {
        using T = std::ranges::range_value_t<R>;

        if (std::ranges::empty(in))
        {
            return T{};
        }

        return std::sqrt(
            std::transform_reduce(
                std::ranges::begin(in),
                std::ranges::end(in),
                T{},
                std::plus<>{},
                numeric::pow_n<2>) /
            std::ranges::size(in));
    }
};

} // namespace detail

inline constexpr detail::rms_fn rms{};

} // namespace piejam::numeric
