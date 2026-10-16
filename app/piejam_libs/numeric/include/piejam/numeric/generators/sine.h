// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/numeric/constants.h>

#include <cmath>
#include <concepts>

namespace piejam::numeric::generators
{

template <std::floating_point T>
class sine
{
public:
    constexpr sine(
        T frequency,
        T sample_rate,
        T amplitude_ = T{1},
        T phase_ = T{0}) noexcept
        : m_amplitude(amplitude_)
        , m_phase(phase_)
        , m_phase_increment(constants::two_pi<T> * frequency / sample_rate)
    {
    }

    constexpr auto operator()() noexcept -> T
    {
        T value = m_amplitude * std::sin(m_phase);
        m_phase += m_phase_increment;
        if (m_phase >= constants::two_pi<T>)
        {
            m_phase -= constants::two_pi<T>;
        }
        return value;
    }

private:
    T m_amplitude;
    T m_phase;           // current phase in radians
    T m_phase_increment; // 2*pi*frequency/sample_rate
};

} // namespace piejam::numeric::generators
