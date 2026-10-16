// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/numeric/constants.h>

#include <boost/assert.hpp>

#include <cmath>
#include <concepts>

namespace piejam::numeric::generators
{

template <std::floating_point T, T A0, T A1>
class cosine_window
{
public:
    explicit constexpr cosine_window(std::size_t size) noexcept
        : m_size(size)
        , m_phase_increment(constants::two_pi<T> / (size - 1))
    {
        BOOST_ASSERT(size > 1);
    }

    [[nodiscard]]
    constexpr auto operator()() noexcept -> T
    {
        BOOST_ASSERT(m_index < m_size);

        T value = A0 - A1 * std::cos(m_phase);
        ++m_index;
        m_phase += m_phase_increment;
        return value;
    }

private:
    std::size_t m_size{};
    std::size_t m_index{};
    T m_phase{};
    T m_phase_increment{};
};

template <std::floating_point T = float>
using hamming = cosine_window<T, T{0.54}, T{0.46}>;

template <std::floating_point T = float>
using hann = cosine_window<T, T{0.5}, T{0.5}>;

} // namespace piejam::numeric::generators
