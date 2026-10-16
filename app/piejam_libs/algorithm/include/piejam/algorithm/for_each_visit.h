// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <ranges>
#include <variant>

namespace piejam::algorithm
{

template <std::ranges::input_range Range, class Visitor>
constexpr auto
for_each_visit(Range&& rng, Visitor&& v) -> Visitor&&
{
    for (auto&& var : std::forward<Range>(rng))
    {
        std::visit(v, std::forward<decltype(var)>(var));
    }

    return std::forward<Visitor>(v);
}

} // namespace piejam::algorithm
