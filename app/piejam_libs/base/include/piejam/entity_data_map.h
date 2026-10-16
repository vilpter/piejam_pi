// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/entity_id.h>

#include <boost/assert.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/stl_interfaces/iterator_interface.hpp>

#include <memory>

namespace piejam
{

template <class Data>
using cached_entity_data_ptr = std::shared_ptr<Data const>;

template <class Id, class Data>
class entity_data_map
{
    using map_t = boost::container::flat_map<Id, std::shared_ptr<Data>>;

public:
    using id_type = Id;
    using data_type = Data;

    using base_iterator = typename map_t::const_iterator;
    struct const_iterator
        : public boost::stl_interfaces::proxy_iterator_interface<
              typename base_iterator::iterator_category,
              std::pair<id_type const&, data_type const&>>
    {
        using base_t = boost::stl_interfaces::proxy_iterator_interface<
            typename base_iterator::iterator_category,
            std::pair<id_type const&, data_type const&>>;

        const_iterator() noexcept = default;

        const_iterator(base_iterator it) noexcept
            : m_it{std::move(it)}
        {
        }

        auto operator->() const noexcept -> typename base_t::pointer
        {
            return typename base_t::pointer{*(*this)};
        }

        auto operator*() const noexcept -> typename base_t::reference
        {
            return {m_it->first, *m_it->second};
        }

    private:
        friend boost::stl_interfaces::access;

        auto base_reference() noexcept -> base_iterator&
        {
            return m_it;
        }

        auto base_reference() const noexcept -> base_iterator
        {
            return m_it;
        }

        base_iterator m_it;
    };

    [[nodiscard]]
    auto empty() const noexcept
    {
        return m_map.empty();
    }

    [[nodiscard]]
    auto size() const noexcept
    {
        return m_map.size();
    }

    [[nodiscard]]
    auto begin() const noexcept
    {
        return const_iterator{m_map.begin()};
    }

    [[nodiscard]]
    auto end() const noexcept
    {
        return const_iterator{m_map.end()};
    }

    [[nodiscard]]
    auto contains(id_type id) const noexcept
    {
        return m_map.contains(id);
    }

    [[nodiscard]]
    auto find(id_type id) const noexcept -> data_type const*
    {
        auto it = m_map.find(id);
        return it != m_map.end() ? it->second.get() : nullptr;
    }

    [[nodiscard]]
    auto cached(id_type id) const noexcept -> cached_entity_data_ptr<data_type>
    {
        auto it = m_map.find(id);
        return it != m_map.end() ? it->second : nullptr;
    }

    [[nodiscard]]
    auto at(id_type id) const noexcept -> data_type const&
    {
        auto it = m_map.find(id);
        BOOST_ASSERT(it != m_map.end());
        return *it->second;
    }

    auto operator==(entity_data_map const&) const noexcept -> bool = default;

    void assign(id_type id, data_type value)
    {
        auto it = m_map.find(id);
        BOOST_ASSERT(it != m_map.end());
        (*it->second) = std::move(value);
    }

    auto insert(id_type id, data_type value) -> bool
    {
        return m_map
            .insert(
                std::pair{id, std::make_shared<data_type>(std::move(value))})
            .second;
    }

    template <class... Args>
    auto emplace(id_type id, Args&&... args) -> bool
    {
        return m_map
            .emplace(
                id,
                std::make_shared<data_type>(std::forward<Args>(args)...))
            .second;
    }

    void erase(id_type id)
    {
        m_map.erase(id);
    }

private:
    map_t m_map;
};

template <class Entity, class Data>
auto
insert(entity_data_map<entity_id<Entity>, std::decay_t<Data>>& m, Data&& value)
    -> entity_id<Entity>
{
    auto id = entity_id<Entity>::generate();
    BOOST_VERIFY(m.insert(id, std::forward<Data>(value)));
    return id;
}

} // namespace piejam
