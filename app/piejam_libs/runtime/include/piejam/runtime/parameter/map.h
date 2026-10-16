// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/parameter/fwd.h>

#include <piejam/enum.h>
#include <piejam/lean_map_facade.h>

#include <boost/assert.hpp>
#include <boost/container/flat_map.hpp>

#include <typeindex>
#include <variant>

namespace piejam::runtime::parameter
{

template <class>
class map;

template <class... ParameterId>
class map<std::variant<ParameterId...>>
{
    using parameter_id_t = std::variant<ParameterId...>;
    using map_t =
        lean_map_facade<boost::container::flat_map<key, parameter_id_t>>;

public:
    map() noexcept = default;

    map(std::initializer_list<std::pair<key, parameter_id_t>> params)
        : m_map{std::move(params)}
    {
    }

    template <scoped_enum<key> E>
    map(std::in_place_type_t<E>,
        std::initializer_list<std::pair<E, parameter_id_t>> params)
        : m_key_type{typeid(E)}
        , m_map{params | std::views::transform([](auto const& p) {
                    return std::pair{std::to_underlying(p.first), p.second};
                })}
    {
    }

    [[nodiscard]]
    auto size() const noexcept -> std::size_t
    {
        return m_map.size();
    }

    [[nodiscard]]
    auto begin() const noexcept
    {
        return m_map.begin();
    }

    [[nodiscard]]
    auto end() const noexcept
    {
        return m_map.end();
    }

    // for unit tests
    template <scoped_enum<key> E>
    [[nodiscard]]
    auto key_type_is() const noexcept -> bool
    {
        return m_key_type == typeid(E);
    }

    [[nodiscard]]
    auto contains(key key) const noexcept -> bool
    {
        return m_map.contains(key);
    }

    template <scoped_enum<key> E>
    [[nodiscard]]
    auto contains(E key) const noexcept -> bool
    {
        BOOST_ASSERT(m_key_type == typeid(E));
        return contains(std::to_underlying(key));
    }

    auto emplace(key key, parameter_id_t id) -> map&
    {
        BOOST_VERIFY(m_map.emplace(key, id).second);
        return *this;
    }

    template <scoped_enum<key> E>
    auto emplace(E key, parameter_id_t id) -> map&
    {
        BOOST_ASSERT(m_key_type == typeid(E));
        return emplace(std::to_underlying(key), id);
    }

    auto find(key key) const noexcept
    {
        return m_map.find(key);
    }

    auto at(key key) const -> parameter_id_t
    {
        return m_map.at(key);
    }

    template <scoped_enum<key> E>
    auto at(E key) const -> parameter_id_t
    {
        BOOST_ASSERT(m_key_type == typeid(E));
        return m_map.at(std::to_underlying(key));
    }

    template <class P>
    auto get(key key) const -> P
    {
        return std::get<P>(m_map.at(key));
    }

    template <class P, scoped_enum<key> E>
    auto get(E key) const -> P
    {
        BOOST_ASSERT(m_key_type == typeid(E));
        return std::get<P>(m_map.at(std::to_underlying(key)));
    }

private:
    std::type_index m_key_type{typeid(key)};
    map_t m_map;
};

} // namespace piejam::runtime::parameter
