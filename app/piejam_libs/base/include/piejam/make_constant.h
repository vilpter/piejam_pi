// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/float_constant.h>

#include <type_traits>

namespace piejam
{

template <auto V>
using make_constant = decltype([]() {
    if constexpr (std::is_same_v<decltype(V), bool>)
    {
        return std::bool_constant<V>{};
    }
    else if constexpr (std::is_floating_point_v<decltype(V)>)
    {
        return float_constant<decltype(V), V>{};
    }
    else
    {
        return std::integral_constant<decltype(V), V>{};
    }
}());

} // namespace piejam
