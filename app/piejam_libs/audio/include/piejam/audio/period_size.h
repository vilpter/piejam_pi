// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <boost/container/static_vector.hpp>

#include <array>

namespace piejam::audio
{

class period_size
{
public:
    constexpr period_size() noexcept = default;

    explicit constexpr period_size(unsigned const value) noexcept
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
    constexpr auto value() const noexcept -> unsigned
    {
        return m_value;
    }

    constexpr auto operator==(period_size const&) const noexcept
        -> bool = default;

private:
    unsigned m_value{};
};

inline constexpr period_size min_period_size{48u};
inline constexpr period_size max_period_size{1024u};
inline constexpr unsigned period_size_step{16u};
inline constexpr std::array preferred_period_sizes =
    []<std::size_t... I>(std::index_sequence<I...>) {
        return std::array{period_size(
            static_cast<unsigned>(I) * period_size_step +
            min_period_size.value())...};
    }(std::make_index_sequence<
        (max_period_size.value() - min_period_size.value()) / period_size_step +
        1u>());

using period_sizes_t =
    boost::container::static_vector<period_size, preferred_period_sizes.size()>;

} // namespace piejam::audio
