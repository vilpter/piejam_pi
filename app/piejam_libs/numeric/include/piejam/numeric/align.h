// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <boost/assert.hpp>

#include <concepts>
#include <limits>
#include <type_traits>

namespace piejam::numeric
{

struct align_down_fn
{
    template <std::unsigned_integral T, std::unsigned_integral N>
    [[nodiscard]]
    constexpr auto operator()(T x, N n) const noexcept
        -> std::common_type_t<T, N>
    {
        BOOST_ASSERT(n != 0);
        using CT = std::common_type_t<T, N>;
        static_assert(std::unsigned_integral<CT>);
        return CT{x} - (CT{x} % CT{n});
    }
};

inline constexpr align_down_fn align_down{};

struct align_up_fn
{
    template <std::unsigned_integral T, std::unsigned_integral N>
    [[nodiscard]]
    constexpr auto operator()(T x, N n) const noexcept
        -> std::common_type_t<T, N>
    {
        BOOST_ASSERT(n != 0);
        using CT = std::common_type_t<T, N>;
        BOOST_ASSERT(x <= std::numeric_limits<CT>::max() - CT{n} + 1);
        CT const adjusted = CT{x} + CT{n} - CT{1};
        return align_down(adjusted, CT{n});
    }
};

inline constexpr align_up_fn align_up{};

struct align_nearest_fn
{
    template <std::unsigned_integral T, std::unsigned_integral N>
    [[nodiscard]]
    constexpr auto operator()(T x, N n) const noexcept
        -> std::common_type_t<T, N>
    {
        BOOST_ASSERT(n != 0);
        using CT = std::common_type_t<T, N>;
        CT const half_n = CT{n} / CT{2};
        BOOST_ASSERT(x <= std::numeric_limits<CT>::max() - half_n);
        CT const adjusted = CT{x} + half_n;
        return align_down(adjusted, CT{n});
    }
};

inline constexpr align_nearest_fn align_nearest{};

} // namespace piejam::numeric
