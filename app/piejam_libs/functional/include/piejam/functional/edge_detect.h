// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/functional/in_interval.h>

#include <boost/hof/returns.hpp>

namespace piejam
{

struct
{
    template <class T, class P, class C>
    [[nodiscard]]
    constexpr auto operator()(T&& threshold, P&& prev, C&& curr) const
        BOOST_HOF_RETURNS(in_right_open(
            std::forward<T>(threshold),
            std::forward<P>(prev),
            std::forward<C>(curr)))
} const rising_edge;

struct
{
    template <class T, class P, class C>
    [[nodiscard]]
    constexpr auto operator()(T&& threshold, P&& prev, C&& curr) const
        BOOST_HOF_RETURNS(in_left_open(
            std::forward<T>(threshold),
            std::forward<C>(curr),
            std::forward<P>(prev)))
} const falling_edge;

} // namespace piejam
