// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <concepts>

namespace piejam
{

template <std::floating_point T, T V, class Tag = void>
struct float_constant
{
    using value_type = T;
    using type = float_constant;

    static constexpr value_type value = V;

    constexpr operator value_type() const noexcept
    {
        return value;
    }

    constexpr auto operator()() const noexcept -> value_type
    {
        return value;
    }
};

template <auto F, class Tag = void>
    requires std::floating_point<decltype(F)>
using make_float_constant = float_constant<decltype(F), F, Tag>;

} // namespace piejam
