// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <boost/hof/returns.hpp>

#include <functional>

namespace piejam
{

namespace detail
{

template <class LeftCompare, class RightCompare>
struct interval_check
{
    template <class T, class L, class H>
    [[nodiscard]]
    constexpr auto operator()(T&& v, L&& lo, H&& hi) const BOOST_HOF_RETURNS(
            LeftCompare{}(std::forward<L>(lo), v) &&
            RightCompare{}(v, std::forward<H>(hi)))

    template <class T>
    [[nodiscard]]
    constexpr auto operator()(T&& v) const noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>)
    {
        return [v = std::forward<T>(v)]<class L, class H>(L&& lo, H&& hi) BOOST_HOF_RETURNS(
            interval_check<LeftCompare, RightCompare>{}(v, std::forward<L>(lo), std::forward<H>(hi))
        );
    }
};

} // namespace detail

inline constexpr auto in_closed =
        detail::interval_check<std::less_equal<>, std::less_equal<>>{};

inline constexpr auto in_open =
        detail::interval_check<std::less<>, std::less<>>{};

inline constexpr auto in_left_open =
        detail::interval_check<std::less<>, std::less_equal<>>{};

inline constexpr auto in_right_open =
        detail::interval_check<std::less_equal<>, std::less<>>{};

} // namespace piejam
