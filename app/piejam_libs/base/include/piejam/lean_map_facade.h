// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <boost/assert.hpp>

#include <ranges>
#include <utility>

namespace piejam
{

template <class Map>
class lean_map_facade
{
public:
    using key_type = typename Map::key_type;
    using mapped_type = typename Map::mapped_type;
    using size_type = typename Map::size_type;
    using const_iterator = typename Map::const_iterator;
    using iterator = typename Map::iterator;

    constexpr lean_map_facade() noexcept = default;

    constexpr lean_map_facade(
        std::initializer_list<std::pair<key_type, mapped_type>> init)
        : m_map{std::move(init)}
    {
    }

    template <std::ranges::range R>
    constexpr lean_map_facade(R&& r)
        : m_map(std::ranges::begin(r), std::ranges::end(r))
    {
    }

    [[nodiscard]]
    constexpr auto empty() const noexcept -> bool
    {
        return m_map.empty();
    }

    [[nodiscard]]
    constexpr auto size() const noexcept -> size_type
    {
        return m_map.size();
    }

    [[nodiscard]]
    constexpr auto begin() const noexcept -> const_iterator
    {
        return m_map.begin();
    }

    [[nodiscard]]
    constexpr auto begin() noexcept -> iterator
    {
        return m_map.begin();
    }

    [[nodiscard]]
    constexpr auto end() const noexcept -> const_iterator
    {
        return m_map.end();
    }

    [[nodiscard]]
    constexpr auto end() noexcept -> iterator
    {
        return m_map.end();
    }

    [[nodiscard]]
    constexpr auto contains(key_type const& key) const noexcept -> bool
    {
        return m_map.contains(key);
    }

    [[nodiscard]]
    constexpr auto find(key_type const& key) const noexcept
        -> mapped_type const*
    {
        auto it = m_map.find(key);
        return it != m_map.end() ? &it->second : nullptr;
    }

    [[nodiscard]]
    constexpr auto find(key_type const& key) noexcept -> mapped_type*
    {
        auto it = m_map.find(key);
        return it != m_map.end() ? &it->second : nullptr;
    }

    [[nodiscard]]
    constexpr auto at(key_type const& key) const noexcept -> mapped_type const&
    {
        auto it = m_map.find(key);
        BOOST_ASSERT(it != m_map.end());
        return it->second;
    }

    [[nodiscard]]
    constexpr auto at(key_type const& key) noexcept -> mapped_type&
    {
        auto it = m_map.find(key);
        BOOST_ASSERT(it != m_map.end());
        return it->second;
    }

    template <class... Args>
    constexpr auto emplace(Args&&... args) noexcept(
        noexcept(m_map.emplace(std::forward<Args>(args)...))) -> decltype(auto)
    {
        return m_map.emplace(std::forward<Args>(args)...);
    }

    constexpr auto
    erase(key_type const& key) noexcept(noexcept(m_map.erase(key)))
        -> decltype(auto)
    {
        return m_map.erase(key);
    }

    constexpr auto operator==(lean_map_facade const&) const noexcept
        -> bool = default;

private:
    Map m_map;
};

} // namespace piejam
