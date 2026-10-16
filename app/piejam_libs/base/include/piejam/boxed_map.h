// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/box.h>
#include <piejam/lean_map_facade.h>

namespace piejam
{

template <class Map>
class boxed_map
{
    using map_t = lean_map_facade<Map>;

public:
    using key_type = typename map_t::key_type;
    using mapped_type = typename map_t::mapped_type;
    using size_type = typename map_t::size_type;
    using const_iterator = typename map_t::const_iterator;
    using iterator = typename map_t::iterator;

    [[nodiscard]]
    auto empty() const noexcept -> bool
    {
        return m_map->empty();
    }

    [[nodiscard]]
    auto size() const noexcept -> size_type
    {
        return m_map->size();
    }

    [[nodiscard]]
    auto begin() const noexcept -> const_iterator
    {
        return m_map->begin();
    }

    [[nodiscard]]
    auto end() const noexcept -> const_iterator
    {
        return m_map->end();
    }

    [[nodiscard]]
    auto contains(key_type const& key) const noexcept -> bool
    {
        return m_map->contains(key);
    }

    [[nodiscard]]
    auto find(key_type const& key) const noexcept -> mapped_type const*
    {
        return m_map->find(key);
    }

    [[nodiscard]]
    auto at(key_type const& key) const noexcept -> mapped_type const&
    {
        return m_map->at(key);
    }

    class locked
    {
    public:
        explicit locked(boxed_map& m)
            : m_locked{m.m_map.lock()}
        {
        }

        [[nodiscard]]
        auto begin() noexcept -> iterator
        {
            return m_locked->begin();
        }

        [[nodiscard]]
        auto end() noexcept -> iterator
        {
            return m_locked->end();
        }

        [[nodiscard]]
        auto find(key_type const& key) -> mapped_type*
        {
            return m_locked->find(key);
        }

        [[nodiscard]]
        auto at(key_type const& key) -> mapped_type&
        {
            return m_locked->at(key);
        }

        template <class... Args>
        auto emplace(Args&&... args)
        {
            return m_locked->emplace(std::forward<Args>(args)...);
        }

        auto erase(key_type const& key)
        {
            return m_locked->erase(key);
        }

    private:
        box<map_t>::write_lock m_locked;
    };

    auto lock() -> locked
    {
        return locked{*this};
    }

    template <class Value>
    auto assign(key_type const& key, Value&& value)
    {
        lock().at(key) = std::forward<Value>(value);
    }

    template <class... Args>
    auto emplace(Args&&... args)
    {
        return lock().emplace(std::forward<Args>(args)...);
    }

    auto erase(key_type const& key)
    {
        return lock().erase(key);
    }

    auto operator==(boxed_map const&) const noexcept -> bool = default;

private:
    box<map_t> m_map;
};

} // namespace piejam
