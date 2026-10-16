// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/numeric/mipp.h>

namespace piejam::numeric::simd
{

namespace detail
{

struct clamp_fn
{
    template <mipp_number T>
    [[nodiscard]]
    constexpr auto
    operator()(mipp::Reg<T> v, mipp::Reg<T> lo, mipp::Reg<T> hi) const noexcept
        -> mipp::Reg<T>
    {
        return mipp::max(lo, mipp::min(hi, v));
    }

    template <mipp_number T>
    [[nodiscard]]
    constexpr auto operator()(mipp::Reg<T> lo, mipp::Reg<T> hi) const noexcept
    {
        return [=](mipp::Reg<T> v) { return clamp_fn{}(v, lo, hi); };
    }

    template <mipp_number T>
    [[nodiscard]]
    constexpr auto operator()(T lo, T hi) const noexcept
    {
        return (*this)(mipp::Reg<T>(lo), mipp::Reg<T>(hi));
    }
};

} // namespace detail

inline constexpr detail::clamp_fn clamp{};

} // namespace piejam::numeric::simd
