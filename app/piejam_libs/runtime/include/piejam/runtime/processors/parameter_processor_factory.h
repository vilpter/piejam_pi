// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/parameter/fwd.h>

#include <piejam/audio/engine/value_io_processor.h>
#include <piejam/entity_id_hash.h>

#include <boost/assert.hpp>
#include <boost/mp11/list.hpp>
#include <boost/mp11/tuple.hpp>

#include <concepts>
#include <memory>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <variant>

namespace piejam::runtime::processors
{

template <class... Parameter>
class parameter_processor_factory
{
public:
    template <class P>
    using parameter_processor =
        audio::engine::value_io_processor<parameter::value_type_t<P>>;

    template <class P>
    using processor_map = std::unordered_map<
        parameter::id_t<P>,
        std::weak_ptr<parameter_processor<P>>>;

    template <class P>
    auto
    find_or_make_processor(parameter::id_t<P> id, std::string_view name = {})
        -> std::shared_ptr<parameter_processor<P>>
    {
        if (auto proc = find_processor(id); proc)
        {
            return proc;
        }

        return make_processor(id, name);
    }

    template <class... P>
    auto find_or_make_processor(
        std::variant<parameter::id_t<P>...> const& param_id,
        std::string_view const name = {})
        -> std::shared_ptr<audio::engine::processor>
    {
        return std::visit(
            [this, name]<class Param>(parameter::id_t<Param> param_id)
                -> std::shared_ptr<audio::engine::processor> {
                return find_or_make_processor<Param>(std::move(param_id), name);
            },
            param_id);
    }

    template <class FindValue>
    void initialize(FindValue&& find_value) const
    {
        boost::mp11::tuple_for_each(m_procs, [&](auto&& procs) {
            for (auto&& [id, weak_proc] : procs)
            {
                if (auto proc = weak_proc.lock())
                {
                    if (auto const value = find_value(id); value)
                    {
                        proc->set(*value);
                    }
                }
            }
        });
    }

    template <class P, std::convertible_to<parameter::value_type_t<P>> V>
    void set(parameter::id_t<P> id, V&& value) const
    {
        if (auto proc = find_processor(id))
        {
            proc->set(std::forward<V>(value));
        }
    }

    template <class P, class F>
    auto consume(parameter::id_t<P> id, F&& f) const
    {
        if (auto proc = find_processor(id))
        {
            proc->consume(std::forward<F>(f));
        }
    }

    void clear_expired()
    {
        (clear_expired<Parameter>(), ...);
    }

private:
    template <class P>
    auto make_processor(parameter::id_t<P> id, std::string_view name = {})
        -> std::shared_ptr<parameter_processor<P>>
    {
        BOOST_ASSERT(id.valid());
        auto proc = std::make_shared<parameter_processor<P>>(name);
        std::get<processor_map<P>>(m_procs).insert_or_assign(id, proc);
        return proc;
    }

    template <class P>
    auto find_processor(parameter::id_t<P> id) const
        -> std::shared_ptr<parameter_processor<P>>
    {
        auto const& map = std::get<processor_map<P>>(m_procs);
        auto it = map.find(id);
        return it != map.end() ? it->second.lock() : nullptr;
    }

    template <class P>
    static bool expired(typename processor_map<P>::value_type const& p) noexcept
    {
        return p.second.expired();
    }

    template <class P>
    void clear_expired()
    {
        std::erase_if(std::get<processor_map<P>>(m_procs), &expired<P>);
    }

    std::tuple<processor_map<Parameter>...> m_procs;
};

} // namespace piejam::runtime::processors
