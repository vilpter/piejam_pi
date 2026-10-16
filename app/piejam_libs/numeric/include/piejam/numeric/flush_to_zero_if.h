// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <boost/hof/returns.hpp>

#include <concepts>
#include <functional>

namespace piejam::numeric
{

template <class T, std::predicate<T> P>
[[nodiscard]]
constexpr auto flush_to_zero_if(T value, P&& p)
    BOOST_HOF_RETURNS(std::invoke(std::forward<P>(p), value) ? T{0} : value);

} // namespace piejam::numeric
