// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/entity_id.h>
#include <piejam/fwd.h>

#include <boost/assert.hpp>
#include <boost/container/flat_map.hpp>

namespace piejam
{

template <class Entity>
class entity_map
{
public:
    using id_t = entity_id<Entity>;
    using map_t = boost::container::flat_map<id_t, Entity>;
    using key_type = typename map_t::key_type;
    using mapped_type = typename map_t::mapped_type;
    using value_type = typename map_t::value_type;
    using size_type = typename map_t::size_type;
    using const_iterator = typename map_t::const_iterator;
    using iterator = typename map_t::const_iterator;

    [[nodiscard]]
    auto empty() const noexcept -> bool
    {
        return m_map.empty();
    }

    [[nodiscard]]
    auto size() const noexcept -> size_type
    {
        return m_map.size();
    }

    [[nodiscard]]
    auto begin() const noexcept -> const_iterator
    {
        return m_map.begin();
    }

    [[nodiscard]]
    auto end() const noexcept -> const_iterator
    {
        return m_map.end();
    }

    [[nodiscard]]
    auto contains(id_t id) const noexcept
    {
        return m_map.contains(id);
    }

    [[nodiscard]]
    auto find(id_t id) const noexcept -> const_iterator
    {
        return m_map.find(id);
    }

    template <class... Args>
    [[nodiscard]]
    auto emplace(Args&&... args) -> id_t
    {
        auto id = id_t::generate();
        m_map.emplace_hint(
            m_map.end(),
            std::piecewise_construct,
            std::forward_as_tuple(id),
            std::forward_as_tuple(std::forward<Args>(args)...));
        return id;
    }

    auto erase(id_t id) -> size_type
    {
        return m_map.erase(id);
    }

    auto operator==(entity_map const&) const noexcept -> bool = default;

private:
    map_t m_map;
};

} // namespace piejam
