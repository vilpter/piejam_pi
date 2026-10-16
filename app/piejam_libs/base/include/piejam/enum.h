// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <type_traits>
#include <utility>

namespace piejam
{

template <class E, class UT>
concept scoped_enum =
    std::is_scoped_enum_v<E> && std::is_same_v<UT, std::underlying_type_t<E>>;

template <class E>
concept bool_enum = scoped_enum<E, bool>;

template <bool_enum E, class T>
[[nodiscard]]
constexpr auto
bool_enum_to(E e, T&& false_value, T&& true_value) noexcept -> decltype(auto)
{
    return static_cast<bool>(e) ? std::forward<T>(true_value)
                                : std::forward<T>(false_value);
}

template <bool_enum ToE, bool_enum FromE>
[[nodiscard]]
constexpr auto
bool_enum_to(FromE e) noexcept -> decltype(auto)
{
    return static_cast<ToE>(static_cast<bool>(e));
}

template <bool_enum E>
[[nodiscard]]
constexpr auto
toggle_bool_enum(E e) noexcept -> E
{
    return static_cast<bool>(e) ? static_cast<E>(false) : static_cast<E>(true);
}

template <bool_enum E>
constexpr auto
toggle_bool_enum_in_place(E& e) noexcept -> E
{
    e = toggle_bool_enum(e);
    return e;
}

} // namespace piejam
