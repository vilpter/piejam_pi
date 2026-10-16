// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <concepts>
#include <functional>
#include <utility>

namespace piejam
{

template <class T, std::predicate<T const&> P, class V>
auto
set_if(T& t, P&& pred, V&& value) noexcept(
    noexcept(std::invoke(std::forward<P>(pred), t)) &&
    noexcept(t = std::forward<V>(value))) -> bool
{
    if (std::invoke(std::forward<P>(pred), t))
    {
        t = std::forward<V>(value);
        return true;
    }

    return false;
}

} // namespace piejam
