// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <boost/mp11/algorithm.hpp>

#include <functional>
#include <tuple>
#include <type_traits>

namespace piejam::tuple
{

template <class Tuple>
using decay_elements_t = boost::mp11::mp_transform<std::decay_t, Tuple>;

template <class Tp, class F>
auto
for_each_while(Tp&& tp, F&& f) -> bool
{
    return std::apply(
        [fn = std::forward<F>(f)](auto&&... elems) mutable {
            return (
                true && ... &&
                std::invoke(fn, std::forward<decltype(elems)>(elems)));
        },
        std::forward<Tp>(tp));
}

template <class Tp, class F>
auto
for_each_until(Tp&& tp, F&& f) -> bool
{
    return std::apply(
        [fn = std::forward<F>(f)](auto&&... elems) mutable {
            return (
                false || ... ||
                std::invoke(fn, std::forward<decltype(elems)>(elems)));
        },
        std::forward<Tp>(tp));
}

} // namespace piejam::tuple
