// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <concepts>

namespace piejam::algorithm
{

namespace detail
{

// Detect if container has erase(it) -> associative
template <class C>
concept has_erase_iterator = requires(C c, typename C::iterator it) {
    { c.erase(it) } -> std::same_as<typename C::iterator>;
};

// Detect if container has erase(first, last) -> sequence
template <class C>
concept has_erase_range = requires(C c, typename C::iterator it) {
    { c.erase(it, it) } -> std::same_as<typename C::iterator>;
};

} // namespace detail

template <
    class Container,
    std::predicate<typename Container::value_type> Predicate>
void
erase_if(Container& c, Predicate pred)
{
    if constexpr (detail::has_erase_range<Container>)
    {
        // Sequence containers (vector, deque, list, boost::vector, etc.)
        auto it = std::remove_if(c.begin(), c.end(), pred);
        c.erase(it, c.end());
    }
    else if constexpr (detail::has_erase_iterator<Container>)
    {
        // Associative containers (map, set, flat_map, etc.)
        for (auto it = c.begin(); it != c.end();)
        {
            if (pred(*it))
            {
                it = c.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    else
    {
        static_assert(
            sizeof(Container) == 0,
            "erase_if: unsupported container type");
    }
}

} // namespace piejam::algorithm
