// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <type_traits>
#include <utility>

namespace piejam
{

template <class T, class... Args>
void
renew(T& t, Args&&... args) noexcept(
    std::is_nothrow_constructible_v<T, Args...>)
{
    if constexpr (std::is_nothrow_constructible_v<T, Args...>)
    {
        // Safe to reconstruct in-place without risk
        t.~T();
        ::new (static_cast<void*>(&t)) T(std::forward<Args>(args)...);
    }
    else
    {
        // Fallback to assignment if constructor might throw
        t = T{std::forward<Args>(args)...};
    }
}

} // namespace piejam
