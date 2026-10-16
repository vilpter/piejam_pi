// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <ranges>
#include <vector>

namespace piejam::algorithm
{

template <std::ranges::input_range Range, class F>
[[nodiscard]]
auto
transform_to_vector(Range&& rng, F&& f)
{
    return std::views::transform(std::forward<Range>(rng), std::forward<F>(f)) |
           std::ranges::to<std::vector>();
}

} // namespace piejam::algorithm
