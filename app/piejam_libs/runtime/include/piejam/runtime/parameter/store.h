// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/parameter/fwd.h>

#include <piejam/enum.h>
#include <piejam/lean_map_facade.h>

#include <boost/container/flat_map.hpp>

#include <memory>
#include <typeindex>

namespace piejam::runtime::parameter
{

class store
{
public:
    template <class Parameter>
    struct slot
    {
        using parameter_type = Parameter;
        using value_type = parameter::value_type_t<Parameter>;
        using cached_type = std::shared_ptr<value_type const>;

        explicit slot(Parameter&& param)
            : m_param{std::move(param)}
        {
        }

        explicit slot(Parameter const& param)
            : m_param{param}
        {
        }

        [[nodiscard]]
        auto param() const noexcept -> Parameter const&
        {
            return m_param;
        }

        [[nodiscard]]
        auto get() const noexcept -> value_type
        {
            return *m_value;
        }

        template <scoped_enum<value_type> E>
        [[nodiscard]]
        auto as() const noexcept -> E
        {
            BOOST_ASSERT(m_param.type == typeid(E));
            return static_cast<E>(*m_value);
        }

        [[nodiscard]]
        auto cached() const noexcept -> cached_type
        {
            return m_value;
        }

        void set(value_type value) noexcept
        {
            *m_value = value;
        }

        auto operator==(slot const& other) const noexcept -> bool
        {
            return m_param == other.m_param && get() == other.get();
        }

    private:
        Parameter m_param;
        std::shared_ptr<value_type> m_value{
            std::make_shared<value_type>(m_param.default_value)};
    };

    template <class P>
    auto emplace(id_t<P> const id, P&& param)
    {
        return BOOST_VERIFY(
            get_map<P>().emplace(id, slot<P>{std::forward<P>(param)}).second);
    }

    template <class P>
    auto remove(id_t<P> const id) -> void
    {
        get_map<P>().erase(id);
    }

    template <class P>
    auto contains(id_t<P> const id) const noexcept -> bool
    {
        return get_map<P>().contains(id);
    }

    template <class P>
    auto find(id_t<P> const id) const noexcept -> slot<P> const*
    {
        return get_map<P>().find(id);
    }

    template <class P>
    auto find(id_t<P> const id) noexcept -> slot<P>*
    {
        return get_map<P>().find(id);
    }

    template <class P>
    auto at(id_t<P> const id) const noexcept -> slot<P> const&
    {
        return get_map<P>().at(id);
    }

    template <class P>
    auto at(id_t<P> const id) noexcept -> slot<P>&
    {
        return get_map<P>().at(id);
    }

    auto operator==(store const& other) const noexcept -> bool = default;

private:
    template <class P>
    using map_t = lean_map_facade<boost::container::flat_map<id_t<P>, slot<P>>>;

    template <class P>
    auto get_map() const -> map_t<P> const&
    {
        return *static_cast<map_t<P> const*>(m_maps.at(typeid(P)).get());
    }

    template <class P>
    auto get_map() -> map_t<P>&
    {
        auto it = m_maps.find(typeid(P));

        if (it == m_maps.end()) [[unlikely]]
        {
            it = m_maps
                     .emplace(
                         std::type_index{typeid(P)},
                         std::make_shared<map_t<P>>())
                     .first;
        }

        return *static_cast<map_t<P>*>(it->second.get());
    }

    boost::container::flat_map<std::type_index, std::shared_ptr<void>> m_maps;
};

} // namespace piejam::runtime::parameter
