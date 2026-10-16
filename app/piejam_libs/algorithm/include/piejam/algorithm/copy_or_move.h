// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <iterator>
#include <ranges>
#include <type_traits>

namespace piejam::algorithm
{

template <
    std::ranges::input_range Input,
    std::output_iterator<std::ranges::range_value_t<Input>> OutputIt>
constexpr auto
copy_or_move(Input&& input, OutputIt out)
{
    using value_t = std::ranges::range_value_t<Input>;

    if constexpr (
        std::is_move_constructible_v<value_t> &&
        !std::is_copy_constructible_v<value_t>)
    {
        return std::ranges::move(std::forward<Input>(input), std::move(out));
    }
    else
    {
        return std::ranges::copy(std::forward<Input>(input), std::move(out));
    }
}

template <
    std::input_iterator InputIt,
    std::sentinel_for<InputIt> Sent,
    std::output_iterator<std::iter_value_t<InputIt>> OutputIt>
constexpr auto
copy_or_move(InputIt first, Sent last, OutputIt out)
{
    return copy_or_move(
        std::ranges::subrange(std::move(first), std::move(last)),
        std::move(out));
}

} // namespace piejam::algorithm
