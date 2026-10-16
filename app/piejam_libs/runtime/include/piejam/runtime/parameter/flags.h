// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/enum.h>

#include <boost/assert.hpp>

#include <bitset>
#include <initializer_list>
#include <utility>

namespace piejam::runtime::parameter
{

template <class E>
concept flags_enum = scoped_enum<E, std::size_t>;

struct flags_set
{
    static constexpr std::size_t max_num_flags{64};

    constexpr flags_set() noexcept = default;

    constexpr auto operator==(flags_set const&) const noexcept
        -> bool = default;

    template <flags_enum E>
    constexpr flags_set(std::initializer_list<E> fs) noexcept
    {
        for (auto f : fs)
        {
            set(f);
        }
    }

    template <flags_enum E>
    constexpr auto set(E f) noexcept -> flags_set&
    {
        BOOST_ASSERT(std::to_underlying(f) < max_num_flags);
        m_fs.set(std::to_underlying(f));
        return *this;
    }

    template <flags_enum E>
    constexpr auto test(E f) const noexcept -> bool
    {
        BOOST_ASSERT(std::to_underlying(f) < max_num_flags);
        return m_fs.test(std::to_underlying(f));
    }

private:
    std::bitset<max_num_flags> m_fs{};
};

} // namespace piejam::runtime::parameter
