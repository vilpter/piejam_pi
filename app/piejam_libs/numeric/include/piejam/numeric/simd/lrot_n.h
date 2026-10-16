// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/numeric/mipp.h>

#include <boost/assert.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/iteration/local.hpp>

namespace piejam::numeric::simd
{

namespace detail
{

struct lrot_n_fn
{
    template <mipp_number T>
    [[nodiscard]]
    constexpr auto operator()(mipp::Reg<T> reg, std::size_t n) const noexcept
    {
#define PIEJAM_LROT_N_MAX_LIMIT 16
        n %= mipp::N<T>();

        // clang-format off
    switch (n)
    {
#define BOOST_PP_LOCAL_LIMITS (1, PIEJAM_LROT_N_MAX_LIMIT - 1)
#define BOOST_PP_LOCAL_MACRO(I)                                                    \
        case BOOST_PP_SUB(PIEJAM_LROT_N_MAX_LIMIT, I):                             \
                    reg = mipp::lrot(reg);                                                 \
                    [[fallthrough]];
#include BOOST_PP_LOCAL_ITERATE()
#undef PIEJAM_LROT_N_MAX_LIMIT
        default:
            return reg;
    }
        // clang-format on
    }
};

} // namespace detail

inline constexpr detail::lrot_n_fn lrot_n{};

} // namespace piejam::numeric::simd
