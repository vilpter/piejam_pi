// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <boost/container/static_vector.hpp>

#include <array>
#include <chrono>
#include <cstdint>

namespace piejam::audio
{

class sample_rate
{
public:
    using storage_type = std::uint32_t;

    constexpr sample_rate() noexcept = default;
    explicit constexpr sample_rate(unsigned const value) noexcept
        : m_value(value)
    {
    }

    [[nodiscard]]
    constexpr auto valid() const noexcept -> bool
    {
        return m_value != 0;
    }
    [[nodiscard]]
    constexpr auto invalid() const noexcept -> bool
    {
        return m_value == 0;
    }

    [[nodiscard]]
    constexpr auto value() const noexcept -> storage_type
    {
        return m_value;
    }

    template <class T>
        requires(
            std::is_floating_point_v<T> ||
            (std::is_integral_v<T> && sizeof(T) >= sizeof(std::uint32_t)))
    [[nodiscard]]
    constexpr auto as() const noexcept -> T
    {
        return static_cast<T>(m_value);
    }

    template <class Period = std::ratio<1>, std::floating_point Rep = double>
    constexpr auto
    duration_for_samples(std::size_t const samples) const noexcept
        -> std::chrono::duration<Rep, Period>
    {
        return std::chrono::duration<Rep>(
            (static_cast<Rep>(samples) / static_cast<Rep>(m_value)));
    }

    template <class Rep, class Period>
    constexpr auto samples_for_duration(
        std::chrono::duration<Rep, Period> const& dur) const noexcept
        -> std::size_t
    {
        return static_cast<std::size_t>(
            m_value * std::chrono::duration<double>(dur).count());
    }

    constexpr auto operator==(sample_rate const&) const noexcept
        -> bool = default;

private:
    storage_type m_value{};
};

inline constexpr std::array preferred_sample_rates{
    sample_rate(44100u),
    sample_rate(48000u),
    sample_rate(88200u),
    sample_rate(96000u),
    sample_rate(176400u),
    sample_rate(192000u)};

using sample_rates_t =
    boost::container::static_vector<sample_rate, preferred_sample_rates.size()>;

} // namespace piejam::audio
