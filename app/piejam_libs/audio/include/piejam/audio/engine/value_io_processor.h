// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/audio/engine/event_input_buffers.h>
#include <piejam/audio/engine/event_output_buffers.h>
#include <piejam/audio/engine/event_port.h>
#include <piejam/audio/engine/named_processor.h>
#include <piejam/audio/engine/process_context.h>
#include <piejam/thread/spsc_slot.h>

#include <boost/core/demangle.hpp>

#include <array>
#include <format>

namespace piejam::audio::engine
{

template <class T>
class value_io_processor : public engine::named_processor
{
public:
    using named_processor::named_processor;

    void set(T const x) noexcept
    {
        m_in_value.push(x);
    }

    bool get(T& x) noexcept
    {
        return m_out_value.pull(x);
    }

    template <class F>
    void consume(F&& f)
    {
        m_out_value.consume(std::forward<F>(f));
    }

    auto type_name() const noexcept -> std::string_view override
    {
        constexpr auto last_name = [](std::string_view s) {
            auto r = s.rfind(':');
            return r == std::string_view::npos ? s : s.substr(r + 1);
        };

        static std::string s_type_name{std::format(
            "{}_io",
            last_name(boost::core::demangle(typeid(T).name())))};
        return s_type_name;
    }

    auto num_inputs() const noexcept -> std::size_t override
    {
        return 0;
    }
    auto num_outputs() const noexcept -> std::size_t override
    {
        return 0;
    }

    auto event_inputs() const noexcept -> event_ports override
    {
        static std::array s_ports{event_port(std::in_place_type<T>, "ext_in")};
        return s_ports;
    }

    auto event_outputs() const noexcept -> event_ports override
    {
        static std::array s_ports{event_port(std::in_place_type<T>, "out")};
        return s_ports;
    }

    void process(engine::process_context const& ctx) override
    {
        auto& out = ctx.event_outputs.get<T>(0);

        m_in_value.consume([&out](T const& value) { out.insert(0, value); });

        for (event<T> const& ev : ctx.event_inputs.get<T>(0))
        {
            m_out_value.push(ev.value());
            out.insert(ev.offset(), ev.value());
        }
    }

protected:
    thread::spsc_slot<T> m_in_value;
    thread::spsc_slot<T> m_out_value;
};

} // namespace piejam::audio::engine
