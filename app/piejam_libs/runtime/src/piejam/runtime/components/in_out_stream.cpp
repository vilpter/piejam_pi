// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/components/stream.h>

#include <piejam/runtime/processors/stream_processor_factory.h>

#include <piejam/algorithm/transform_to_vector.h>
#include <piejam/audio/engine/component.h>
#include <piejam/audio/engine/graph.h>
#include <piejam/audio/engine/graph_endpoint.h>
#include <piejam/audio/engine/identity_processor.h>
#include <piejam/audio/engine/stream_processor.h>
#include <piejam/range/indices.h>
#include <piejam/range/iota.h>

#include <vector>

namespace piejam::runtime::components
{

namespace
{

class in_out_stream final : public audio::engine::component
{

public:
    in_out_stream(
        std::unique_ptr<audio::engine::component> comp,
        std::shared_ptr<audio::engine::processor> stream_proc)
        : m_comp{std::move(comp)}
        , m_stream_proc{std::move(stream_proc)}
    {
    }

    auto inputs() const -> endpoints override
    {
        return m_inputs;
    }

    auto outputs() const -> endpoints override
    {
        return m_outputs;
    }

    auto event_inputs() const -> endpoints override
    {
        return {};
    }

    auto event_outputs() const -> endpoints override
    {
        return {};
    }

    void connect(audio::engine::graph& g) const override
    {
        m_comp->connect(g);

        std::size_t port{};
        for (auto& proc : m_identity)
        {
            g.audio.insert(
                {.proc = *proc, .port = 0},
                {.proc = *m_stream_proc, .port = port});

            g.audio.insert({.proc = *proc, .port = 0}, m_comp->inputs()[port]);

            ++port;
        }

        for (auto out : m_comp->outputs())
        {
            g.audio.insert(out, {.proc = *m_stream_proc, .port = port});

            ++port;
        }
    }

private:
    std::unique_ptr<audio::engine::component> m_comp;
    std::shared_ptr<audio::engine::processor> m_stream_proc;

    std::vector<std::unique_ptr<audio::engine::processor>> m_identity{
        algorithm::transform_to_vector(
            range::iota(m_comp->num_inputs()),
            [](auto) { return audio::engine::make_identity_processor(); })};

    std::vector<audio::engine::graph_endpoint> m_inputs{
        algorithm::transform_to_vector(
            range::indices(m_identity),
            [this](auto port) {
                return audio::engine::graph_endpoint{
                    .proc = *m_identity[port],
                    .port = 0};
            })};
    std::vector<audio::engine::graph_endpoint> m_outputs{
        algorithm::transform_to_vector(
            range::iota(m_comp->num_outputs()),
            [this](std::size_t port) { return m_comp->outputs()[port]; })};
};

} // namespace

auto
wrap_with_in_out_stream(
    std::unique_ptr<audio::engine::component> comp,
    std::string_view name,
    runtime::audio_stream_id stream_id,
    runtime::processors::stream_processor_factory& stream_proc_factory,
    std::size_t const buffer_capacity_per_channel)
    -> std::unique_ptr<audio::engine::component>
{
    auto stream_proc = stream_proc_factory.make_processor(
        stream_id,
        comp->num_inputs() + comp->num_outputs(),
        buffer_capacity_per_channel,
        name);
    return std::make_unique<in_out_stream>(
        std::move(comp),
        std::move(stream_proc));
}

} // namespace piejam::runtime::components
