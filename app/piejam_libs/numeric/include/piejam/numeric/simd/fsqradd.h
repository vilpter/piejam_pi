// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/numeric/mipp.h>

namespace piejam::numeric::simd
{

namespace detail
{

struct fsqradd_fn
{
    template <std::floating_point T>
    [[nodiscard]]
    constexpr auto operator()(mipp::Reg<T> v, mipp::Reg<T> a) const noexcept
        -> mipp::Reg<T>
    {
        return mipp::fmadd(v, v, a);
    }
};

} // namespace detail

inline constexpr detail::fsqradd_fn fsqradd{};

} // namespace piejam::numeric::simd
