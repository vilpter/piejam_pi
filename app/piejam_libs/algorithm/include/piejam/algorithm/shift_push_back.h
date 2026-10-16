// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/algorithm/copy_or_move.h>

#include <algorithm>
#include <iterator>
#include <ranges>

namespace piejam::algorithm
{

template <std::ranges::sized_range Target, std::ranges::sized_range Source>
constexpr void
shift_push_back(Target& target, Source&& source)
{
    auto const target_size = std::ranges::size(target);
    auto const source_size = std::ranges::size(source);
    auto const count = std::min(target_size, source_size);

    auto target_begin = std::ranges::begin(target);
    auto target_end = std::ranges::end(target);

    std::shift_left(target_begin, target_end, count);

    auto source_begin = std::ranges::begin(source);
    auto source_end = std::ranges::end(source);

    copy_or_move(
        std::ranges::next(source_begin, source_size - count),
        source_end,
        std::ranges::next(target_begin, target_size - count));
}

} // namespace piejam::algorithm
