// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/audio/engine/smoother_processor.h>

#include <piejam/audio/engine/event_input_buffers.h>
#include <piejam/audio/engine/event_port.h>
#include <piejam/audio/engine/named_processor.h>
#include <piejam/audio/engine/single_event_input_processor.h>
#include <piejam/audio/slice.h>

#include <boost/assert.hpp>

#include <algorithm>

namespace piejam::audio::engine
{

namespace
{

class lut_smoother_processor final
    : public named_processor
    , public single_event_input_processor<lut_smoother_processor, float>
{
public:
    lut_smoother_processor(
        std::span<float const> lut,
        float current,
        std::string_view const name)
        : named_processor{name}
        , m_lut{lut}
        , m_current{current}
        , m_target{current}
    {
        BOOST_ASSERT(m_lut.size() >= 2);
        BOOST_ASSERT(m_lut.front() <= current && current <= m_lut.back());
        BOOST_ASSERT(std::ranges::is_sorted(m_lut));
    }

    auto type_name() const noexcept -> std::string_view override
    {
        return "smooth";
    }

    auto num_inputs() const noexcept -> std::size_t override
    {
        return 0;
    }

    auto num_outputs() const noexcept -> std::size_t override
    {
        return 1;
    }

    auto event_inputs() const noexcept -> event_ports override
    {
        static std::array s_ports{event_port(std::in_place_type<float>, "ev")};
        return s_ports;
    }

    auto event_outputs() const noexcept -> event_ports override
    {
        return {};
    }

    void process(engine::process_context const& ctx) override
    {
        ctx.results[0] = ctx.outputs[0];

        process_sliced(ctx);
    }

    void process_buffer(process_context const& ctx)
    {
        if (is_running())
        {
            generate(ctx.outputs[0]);
        }
        else
        {
            ctx.results[0] = m_current;
        }
    }

    void process_slice(
        process_context const& ctx,
        std::size_t const offset,
        std::size_t const count)
    {
        auto it_out = std::next(ctx.outputs[0].begin(), offset);

        if (is_running())
        {
            generate(ctx.outputs[0].subspan(offset, count));
        }
        else
        {
            std::ranges::fill_n(std::move(it_out), count, m_current);
        }
    }

    void process_event(process_context const&, event<float> const& ev)
    {
        BOOST_ASSERT(m_lut.front() <= ev.value() && ev.value() <= m_lut.back());

        m_target = ev.value();

        if (m_current < m_target)
        {
            m_current_index = idx_up(m_current);
            m_target_index = idx_up(m_target);
        }
        else if (m_target < m_current)
        {
            m_current_index = idx_down(m_current);
            m_target_index = idx_down(m_target);
        }
    }

private:
    [[nodiscard]]
    auto is_running() const noexcept -> bool
    {
        return m_current != m_target;
    }

    // first element > value
    [[nodiscard]]
    auto idx_up(float value) const noexcept -> std::size_t
    {
        return std::ranges::upper_bound(m_lut, value) - m_lut.begin();
    }

    // last element < value
    [[nodiscard]]
    auto idx_down(float value) const noexcept -> std::size_t
    {
        auto it = std::ranges::upper_bound(
            m_lut.rbegin(),
            m_lut.rend(),
            value,
            std::greater<>{});
        return m_lut.size() - std::distance(m_lut.rbegin(), it);
    }

    void generate(std::span<float> out)
    {
        BOOST_ASSERT(!out.empty());

        bool const up = m_current_index < m_target_index;
        bool const down = m_target_index < m_current_index;

        auto num = std::min(
            out.size(),
            up ? m_target_index - m_current_index
               : m_current_index - m_target_index);

        if (up)
        {
            auto first = m_lut.begin() + m_current_index;
            auto last = first + num;
            std::ranges::copy(first, last, out.begin());
            m_current_index += num;
        }
        else if (down)
        {
            auto last = m_lut.begin() + m_current_index;
            auto first = last - num;
            std::ranges::reverse_copy(first, last, out.begin());
            m_current_index -= num;
        }

        if (m_current_index == m_target_index)
        {
            m_current = m_target;

            if (num < out.size())
            {
                std::fill(out.begin() + num, out.end(), m_current);
            }
        }
        else
        {
            m_current = out.back();
        }
    }

    std::span<float const> m_lut;
    float m_current;
    float m_target;
    std::size_t m_current_index{};
    std::size_t m_target_index{};
};

} // namespace

auto
make_lut_smoother_processor(
    std::span<float const> lut,
    float current,
    std::string_view name) -> std::unique_ptr<processor>
{
    return std::make_unique<lut_smoother_processor>(lut, current, name);
}

} // namespace piejam::audio::engine
