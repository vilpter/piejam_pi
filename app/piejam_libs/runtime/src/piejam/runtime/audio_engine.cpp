// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/audio_engine.h>

#include <piejam/runtime/components/make_fx.h>
#include <piejam/runtime/components/mixer_channel.h>
#include <piejam/runtime/components/mute_solo.h>
#include <piejam/runtime/components/solo_switch.h>
#include <piejam/runtime/external_audio.h>
#include <piejam/runtime/fx/module.h>
#include <piejam/runtime/mixer.h>
#include <piejam/runtime/parameter_processor_factory.h>
#include <piejam/runtime/processors/midi_assignment_processor.h>
#include <piejam/runtime/processors/midi_input_processor.h>
#include <piejam/runtime/processors/midi_learn_processor.h>
#include <piejam/runtime/processors/midi_to_parameter_processor.h>
#include <piejam/runtime/processors/stream_processor_factory.h>
#include <piejam/runtime/solo_group.h>
#include <piejam/runtime/state.h>

#include <piejam/algorithm/for_each_adjacent.h>
#include <piejam/algorithm/transform_to_vector.h>
#include <piejam/asserted.h>
#include <piejam/audio/engine/component.h>
#include <piejam/audio/engine/dag.h>
#include <piejam/audio/engine/dag_executor.h>
#include <piejam/audio/engine/graph.h>
#include <piejam/audio/engine/graph_algorithms.h>
#include <piejam/audio/engine/graph_generic_algorithms.h>
#include <piejam/audio/engine/graph_node.h>
#include <piejam/audio/engine/graph_to_dag.h>
#include <piejam/audio/engine/input_processor.h>
#include <piejam/audio/engine/mix_processor.h>
#include <piejam/audio/engine/output_processor.h>
#include <piejam/audio/engine/process.h>
#include <piejam/audio/engine/rt_task_executor.h>
#include <piejam/audio/engine/stream_processor.h>
#include <piejam/audio/engine/value_io_processor.h>
#include <piejam/audio/sample_rate.h>
#include <piejam/enum.h>
#include <piejam/lean_map_facade.h>
#include <piejam/midi/event.h>
#include <piejam/midi/input_event_handler.h>
#include <piejam/range/indices.h>
#include <piejam/range/iota.h>
#include <piejam/thread/configuration.h>

#include <boost/hof/match.hpp>
#include <boost/range/algorithm_ext/erase.hpp>

#include <fstream>
#include <optional>
#include <ranges>

namespace piejam::runtime
{

namespace
{

using processor_ptr = std::unique_ptr<audio::engine::processor>;
using shared_processor_ptr = std::shared_ptr<audio::engine::processor>;

using component_ptr = std::unique_ptr<audio::engine::component>;
using shared_component_ptr = std::shared_ptr<audio::engine::component>;

struct processor_map
{
    processor_ptr midi_input;
    processor_ptr midi_learn;
    processor_ptr midi_assign;

    struct midi_assignment_key
    {
        parameter_id param_id;
        midi_assignment assignment;

        constexpr auto
        operator<=>(midi_assignment_key const&) const noexcept = default;
    };

    struct midi_assignment_processors
    {
        shared_processor_ptr midi_to_param;
        shared_processor_ptr param;
    };

    boost::container::flat_map<midi_assignment_key, midi_assignment_processors>
        midi_assignments;
};

struct component_map
{
    template <class Key>
    using components_t =
        lean_map_facade<boost::container::flat_map<Key, shared_component_ptr>>;

    components_t<mixer::channel_id> mixer_inputs;
    components_t<mixer::channel_id> mixer_outputs;

    struct mixer_aux_send_key
    {
        mixer::channel_id channel_id;
        mixer::channel_id route;

        constexpr auto
        operator<=>(mixer_aux_send_key const&) const noexcept = default;
    };

    components_t<mixer_aux_send_key> mixer_aux_sends;

    component_ptr solo_switch;

    components_t<fx::module_id> fx_modules;
};

template <class T>
using value_io_processor_ptr =
    std::unique_ptr<audio::engine::value_io_processor<T>>;

void
make_mixer_components(
    component_map& comps,
    component_map& prev_comps,
    audio::sample_rate const sample_rate,
    strings_t const& strings,
    mixer::state const& mixer_state,
    parameter::store const& params,
    parameter_processor_factory& param_procs,
    processors::stream_processor_factory& stream_procs)
{
    for (auto const& [mixer_channel_id, mixer_channel] : mixer_state.channels)
    {
        if (auto comp = prev_comps.mixer_inputs.find(mixer_channel_id))
        {
            BOOST_ASSERT(*comp);
            comps.mixer_inputs.emplace(mixer_channel_id, *comp);
        }
        else
        {
            comps.mixer_inputs.emplace(
                mixer_channel_id,
                components::make_mixer_channel_input(mixer_channel));
        }

        if (auto comp = prev_comps.mixer_outputs.find(mixer_channel_id))
        {
            BOOST_ASSERT(*comp);
            comps.mixer_outputs.emplace(mixer_channel_id, *comp);
        }
        else
        {
            comps.mixer_outputs.emplace(
                mixer_channel_id,
                components::make_mixer_channel_output(
                    mixer_channel,
                    *strings.at(mixer_channel.name),
                    param_procs,
                    stream_procs,
                    sample_rate));
        }

        if (auto const* const channel_aux_sends =
                mixer_state.aux_sends.find(mixer_channel_id))
        {
            for (auto const& [aux, aux_send] : *channel_aux_sends)
            {
                if (!params.at(aux_send.active()).get())
                {
                    continue;
                }

                component_map::mixer_aux_send_key const aux_send_key{
                    .channel_id = mixer_channel_id,
                    .route = aux};

                if (auto comp = prev_comps.mixer_aux_sends.find(aux_send_key))
                {
                    BOOST_ASSERT(*comp);
                    comps.mixer_aux_sends.emplace(aux_send_key, *comp);
                }
                else
                {
                    comps.mixer_aux_sends.emplace(
                        aux_send_key,
                        components::make_mixer_channel_aux_send(
                            *strings.at(mixer_channel.name),
                            aux_send.volume(),
                            param_procs));
                }
            }
        }
    }
}

void
make_solo_group_components(
    component_map& comps,
    solo_groups_t const& solo_groups,
    parameter_processor_factory& param_procs)
{
    comps.solo_switch = components::make_solo_switch(solo_groups, param_procs);
}

auto
make_fx_chain_components(
    component_map& comps,
    component_map& prev_comps,
    audio::sample_rate const sample_rate,
    fx::state const& fx_state,
    parameter::store const& params,
    fx::simple_ladspa_processor_factory const& ladspa_fx_proc_factory,
    parameter_processor_factory& param_procs,
    processors::stream_processor_factory& stream_procs)
{
    auto get_fx_param_name =
        [&params](parameter_id param_id) -> std::string_view {
        return std::visit(
            [&](auto typed_param_id) -> std::string_view {
                return *params.at(typed_param_id).param().name;
            },
            param_id);
    };

    for (auto const& [fx_mod_id, fx_mod] : fx_state.modules)
    {
        if (!params.at(fx_state.active_modules.at(fx_mod_id)).get())
        {
            continue;
        }

        if (auto fx_comp = prev_comps.fx_modules.find(fx_mod_id))
        {
            BOOST_ASSERT(*fx_comp);
            comps.fx_modules.emplace(fx_mod_id, *fx_comp);
        }
        else
        {
            auto comp = components::make_fx(
                fx_mod,
                get_fx_param_name,
                ladspa_fx_proc_factory,
                param_procs,
                stream_procs,
                sample_rate);
            if (comp)
            {
                comps.fx_modules.emplace(fx_mod_id, std::move(comp));
            }
        }
    }
}

auto
make_midi_processors(
    std::unique_ptr<midi::input_event_handler> midi_in,
    bool const midi_learning,
    processor_map& procs)
{
    if (midi_in)
    {
        procs.midi_input =
            processors::make_midi_input_processor(std::move(midi_in));

        if (midi_learning)
        {
            procs.midi_learn = processors::make_midi_learn_processor();
        }
    }
}

auto
make_midi_assignment_processors(
    midi_assignments_map const& assignments,
    std::span<parameter_id const> disabled_assignments,
    parameter::store const& params,
    parameter_processor_factory& param_procs,
    processor_map& procs,
    processor_map& prev_procs)
{
    procs.midi_assign = processors::make_midi_assignment_processor(assignments);

    for (auto const& [param_id, assignment] : assignments)
    {
        if (std::ranges::contains(disabled_assignments, param_id))
        {
            continue;
        }

        std::visit(
            [&]<class ParamId>(ParamId const typed_param_id) {
                auto it_prev = prev_procs.midi_assignments.find(
                    {.param_id = typed_param_id, .assignment = assignment});

                if (it_prev != prev_procs.midi_assignments.end())
                {
                    procs.midi_assignments.insert(*it_prev);
                }
                else
                {
                    auto const& param = params.at(typed_param_id).param();

                    procs.midi_assignments.emplace(
                        processor_map::midi_assignment_key{
                            .param_id = typed_param_id,
                            .assignment = assignment},
                        processor_map::midi_assignment_processors{
                            .midi_to_param =
                                processors::make_midi_to_parameter_processor(
                                    assignment,
                                    param),
                            .param = param_procs.find_or_make_processor(
                                typed_param_id)});
                }
            },
            param_id);
    }
}

auto
connect_fx_chain(
    audio::engine::graph& g,
    std::vector<audio::engine::component*> const& fx_chain_comps)
{
    for (auto comp : fx_chain_comps)
    {
        comp->connect(g);
    }

    algorithm::for_each_adjacent(
        fx_chain_comps,
        [&](auto const l_comp, auto const r_comp) {
            connect(g, *l_comp, *r_comp);
        });
}

auto
connect_mixer_channel_with_fx_chain(
    audio::engine::graph& g,
    component_map const& comps,
    audio::engine::component& mixer_channel_in,
    fx::chain_t const& fx_chain,
    audio::engine::component& mixer_channel_out)
{
    auto fx_chain_comps = algorithm::transform_to_vector(
        fx_chain,
        [&comps](fx::module_id fx_mod_id) {
            auto comp = comps.fx_modules.find(fx_mod_id);
            return comp ? comp->get() : nullptr;
        });
    boost::remove_erase(fx_chain_comps, nullptr);

    if (fx_chain_comps.empty())
    {
        connect(g, mixer_channel_in, mixer_channel_out);
    }
    else
    {
        connect(g, mixer_channel_in, *fx_chain_comps.front());
        connect_fx_chain(g, fx_chain_comps);
        connect(g, *fx_chain_comps.back(), mixer_channel_out);
    }
}

void
connect_mixer_input(
    audio::engine::graph& g,
    mixer::state const& mixer_state,
    external_audio::state const& external_audio_state,
    component_map const& comps,
    std::span<std::unique_ptr<audio::engine::input_processor> const> const
        input_procs,
    mixer::channel_id const& mixer_channel_id,
    audio::engine::component& mixer_channel_in)
{
    std::visit(
        boost::hof::match(
            [&](external_audio::device_id const device_id) {
                auto const bus_type =
                    external_audio_state.devices.at(device_id).bus_type;

                auto connect_channel = [&](audio::bus_channel channel,
                                           std::size_t mixer_port) {
                    if (auto const ch = external_audio_state.device_channels.at(
                            {device_id, channel});
                        ch != npos && ch < input_procs.size())
                    {
                        g.audio.insert(
                            audio::engine::src_endpoint(*input_procs[ch], 0),
                            audio::engine::dst_endpoint(
                                mixer_channel_in,
                                mixer_port));
                    }
                };

                if (bus_type == audio::bus_type::mono)
                {
                    connect_channel(audio::bus_channel::mono, 0);
                }
                else
                {
                    connect_channel(audio::bus_channel::left, 0);
                    connect_channel(audio::bus_channel::right, 1);
                }
            },
            [&](mixer::channel_id const src_channel_id) {
                auto& source_mixer_channel_out =
                    asserted::deref(comps.mixer_outputs.at(src_channel_id));

                audio::engine::connect(
                    g,
                    source_mixer_channel_out,
                    mixer_channel_in);
            },
            [](auto const&) {}),
        mixer_state.io_map.at(mixer_channel_id).in());
}

void
connect_mixer_output(
    audio::engine::graph& g,
    mixer::state const& mixer_state,
    external_audio::device_channels_t const& device_channels,
    component_map const& comps,
    std::span<std::unique_ptr<audio::engine::output_processor> const> const
        output_procs,
    mixer::io_address_t const& mixer_channel_out_route,
    audio::engine::component& mixer_channel_out_comp)
{
    std::visit(
        boost::hof::match(
            [&](external_audio::device_id const device_id) {
                auto connect_channel = [&](audio::bus_channel channel,
                                           std::size_t mixer_port) {
                    if (auto const ch =
                            device_channels.at({device_id, channel});
                        ch != npos && ch < output_procs.size())
                    {
                        g.audio.insert(
                            audio::engine::src_endpoint(
                                mixer_channel_out_comp,
                                mixer_port),
                            audio::engine::dst_endpoint(*output_procs[ch], 0));
                    }
                };

                connect_channel(audio::bus_channel::left, 0);
                connect_channel(audio::bus_channel::right, 1);
            },
            [&](mixer::channel_id const dst_channel_id) {
                auto const& dst_channel_in =
                    mixer_state.io_map.at(dst_channel_id).in();

                if (std::holds_alternative<mixer::mix_input>(dst_channel_in))
                {
                    auto& dst_mixer_channel_in_comp =
                        asserted::deref(comps.mixer_inputs.at(dst_channel_id));

                    audio::engine::connect(
                        g,
                        mixer_channel_out_comp,
                        dst_mixer_channel_in_comp);
                }
            },
            [](auto const&) {}),
        mixer_channel_out_route);
}

void
connect_aux_sends(
    audio::engine::graph& g,
    mixer::state const& mixer_state,
    mixer::channel_id mixer_channel_id,
    component_map const& comps,
    parameter::store const& params)
{
    if (auto const* const channel_aux_sends =
            mixer_state.aux_sends.find(mixer_channel_id))
    {
        auto& mixer_channel_out =
            asserted::deref(comps.mixer_outputs.at(mixer_channel_id));

        for (auto const& [aux, aux_send] : *channel_aux_sends)
        {
            auto const mixer_channel_aux_send = comps.mixer_aux_sends.find(
                component_map::mixer_aux_send_key{mixer_channel_id, aux});

            // It is possible that aux send component does not exist if aux send
            // is not active
            if (mixer_channel_aux_send)
            {
                BOOST_ASSERT(*mixer_channel_aux_send);

                mixer_channel_aux_send->get()->connect(g);

                auto aux_send_fader_tap = params.at(aux_send.fader_tap())
                                              .as<mixer::aux_send_fader_tap>();
                auto aux_channel_fader_tap =
                    params
                        .at(mixer_state.aux_channels.at(aux)
                                .default_fader_tap())
                        .as<mixer::aux_channel_fader_tap>();

                auto const [out_L, out_R] = [&]() {
                    switch (aux_send_fader_tap)
                    {
                        case mixer::aux_send_fader_tap::auto_:
                            switch (aux_channel_fader_tap)
                            {
                                case mixer::aux_channel_fader_tap::post:
                                    return std::pair{0uz, 1uz};

                                case mixer::aux_channel_fader_tap::pre:
                                    return std::pair{2uz, 3uz};
                            }
                            break;

                        case mixer::aux_send_fader_tap::post:
                            return std::pair{0uz, 1uz};

                        case mixer::aux_send_fader_tap::pre:
                            return std::pair{2uz, 3uz};
                    }

                    return std::pair{0uz, 1uz};
                }();

                g.audio.insert(
                    audio::engine::src_endpoint(mixer_channel_out, out_L),
                    audio::engine::dst_endpoint(
                        *mixer_channel_aux_send->get(),
                        0));
                g.audio.insert(
                    audio::engine::src_endpoint(mixer_channel_out, out_R),
                    audio::engine::dst_endpoint(
                        *mixer_channel_aux_send->get(),
                        1));

                auto const& dst_channel_in = mixer_state.io_map.at(aux).in();

                if (std::holds_alternative<mixer::mix_input>(dst_channel_in))
                {
                    auto* const dst_mixer_channel_in_comp =
                        comps.mixer_inputs.at(aux).get();
                    BOOST_ASSERT(dst_mixer_channel_in_comp);

                    audio::engine::connect(
                        g,
                        *mixer_channel_aux_send->get(),
                        *dst_mixer_channel_in_comp);
                }
            }
        }
    }
}

auto
make_graph(
    component_map const& comps,
    mixer::state const& mixer_state,
    external_audio::state const& external_audio_state,
    parameter::store const& params,
    std::span<std::unique_ptr<audio::engine::input_processor> const> const
        input_procs,
    std::span<std::unique_ptr<audio::engine::output_processor> const> const
        output_procs)
{
    audio::engine::graph g;

    for (auto const& [mixer_channel_id, mixer_channel] : mixer_state.channels)
    {
        audio::engine::component& mixer_channel_in =
            asserted::deref(comps.mixer_inputs.at(mixer_channel_id));

        audio::engine::component& mixer_channel_out =
            asserted::deref(comps.mixer_outputs.at(mixer_channel_id));

        mixer_channel_in.connect(g);
        mixer_channel_out.connect(g);

        connect_mixer_input(
            g,
            mixer_state,
            external_audio_state,
            comps,
            input_procs,
            mixer_channel_id,
            mixer_channel_in);

        connect_mixer_channel_with_fx_chain(
            g,
            comps,
            mixer_channel_in,
            mixer_state.fx_chains.at(mixer_channel_id),
            mixer_channel_out);

        connect_mixer_output(
            g,
            mixer_state,
            external_audio_state.device_channels,
            comps,
            output_procs,
            mixer_state.io_map.at(mixer_channel_id).out(),
            mixer_channel_out);

        connect_aux_sends(g, mixer_state, mixer_channel_id, comps, params);
    }

    return g;
}

void
connect_midi(
    audio::engine::graph& g,
    processor_map const& procs,
    audio::engine::processor* midi_learn_output_proc,
    midi_assignments_map const& assignments,
    std::span<parameter_id const> disabled_assignments)
{
    auto* const midi_in_proc = procs.midi_input.get();
    if (!midi_in_proc)
    {
        return;
    }

    if (auto* const midi_learn_proc = procs.midi_learn.get())
    {
        BOOST_ASSERT(midi_learn_output_proc);

        g.event.insert({*midi_in_proc, 0}, {*midi_learn_proc, 0});
        g.event.insert({*midi_learn_proc, 0}, {*midi_learn_output_proc, 0});

        if (auto* const midi_assign_proc = procs.midi_assign.get())
        {
            g.event.insert({*midi_learn_proc, 1}, {*midi_assign_proc, 0});
        }
    }
    else if (auto* const midi_assign_proc = procs.midi_assign.get())
    {
        g.event.insert({*midi_in_proc, 0}, {*midi_assign_proc, 0});
    }

    if (auto* const midi_assign_proc = procs.midi_assign.get())
    {
        std::size_t out_index{};
        for (auto const& [id, assignment] : assignments)
        {
            if (std::ranges::contains(disabled_assignments, id))
            {
                continue;
            }

            std::visit(
                [&](auto const& param_id) {
                    auto it_midi_conv_procs = procs.midi_assignments.find(
                        {.param_id = param_id, .assignment = assignment});

                    BOOST_ASSERT(
                        it_midi_conv_procs != procs.midi_assignments.end());

                    processor_map::midi_assignment_processors const&
                        midi_conv_procs = it_midi_conv_procs->second;
                    BOOST_ASSERT(midi_conv_procs.midi_to_param);
                    BOOST_ASSERT(midi_conv_procs.param);

                    g.event.insert(
                        {.proc = *midi_assign_proc, .port = out_index++},
                        {.proc = *midi_conv_procs.midi_to_param, .port = 0});

                    g.event.insert(
                        {.proc = *midi_conv_procs.midi_to_param, .port = 0},
                        {.proc = *midi_conv_procs.param, .port = 0});
                },
                id);
        }
    }
}

void
connect_solo_groups(
    audio::engine::graph& g,
    component_map const& comps,
    solo_groups_t const& solo_groups)
{
    BOOST_ASSERT(comps.solo_switch);

    comps.solo_switch->connect(g);

    BOOST_ASSERT(
        solo_groups.size() == comps.solo_switch->event_outputs().size());
    std::size_t index{};
    for (auto const& id : solo_groups | std::views::keys)
    {
        auto mix_out = comps.mixer_outputs.find(id);
        BOOST_ASSERT(mix_out);

        g.event.insert(
            comps.solo_switch->event_outputs()[index],
            mix_out->get()->event_inputs()[0]);

        ++index;
    }
}

template <class Processor>
auto
make_io_processors(std::size_t const num_channels)
{
    return algorithm::transform_to_vector(
        range::iota(num_channels),
        [](std::size_t const i) {
            return std::make_unique<Processor>(std::to_string(i));
        });
}

} // namespace

struct audio_engine::impl
{
    impl(
        audio::sample_rate const sr,
        std::span<audio::engine::rt_task_executor> const workers,
        std::size_t num_device_input_channels,
        std::size_t num_device_output_channels)
        : sample_rate(sr)
        , worker_threads(workers)
        , input_procs(
              make_io_processors<audio::engine::input_processor>(
                  num_device_input_channels))
        , output_procs(
              make_io_processors<audio::engine::output_processor>(
                  num_device_output_channels))
    {
    }

    audio::sample_rate sample_rate;

    audio::engine::process process;
    std::span<audio::engine::rt_task_executor> worker_threads;

    std::vector<std::unique_ptr<audio::engine::input_processor>> input_procs;
    std::vector<std::unique_ptr<audio::engine::output_processor>> output_procs;

    std::vector<processor_ptr> mixer_procs;
    value_io_processor_ptr<midi::external_event> midi_learn_output_proc;

    processor_map procs;
    component_map comps;

    parameter_processor_factory param_procs;
    processors::stream_processor_factory stream_procs;

    audio::engine::graph graph;
};

audio_engine::audio_engine(
    std::span<audio::engine::rt_task_executor> const workers,
    audio::sample_rate const sample_rate,
    unsigned const num_device_input_channels,
    unsigned const num_device_output_channels)
    : m_impl(
          make_pimpl<impl>(
              sample_rate,
              workers,
              num_device_input_channels,
              num_device_output_channels))
{
}

template <class P>
void
audio_engine::set_parameter_value(
    parameter::id_t<P> const id,
    parameter::value_type_t<P> const value) const
{
    m_impl->param_procs.set(id, value);
}

template void audio_engine::set_parameter_value(bool_parameter_id, bool) const;
template void
audio_engine::set_parameter_value(float_parameter_id, float) const;
template void audio_engine::set_parameter_value(int_parameter_id, int) const;
template void audio_engine::set_parameter_value(enum_parameter_id, int) const;

template <class P>
auto
audio_engine::get_parameter_update(parameter::id_t<P> const id) const
    -> std::optional<parameter::value_type_t<P>>
{
    using value_type = parameter::value_type_t<P>;
    std::optional<value_type> result;
    m_impl->param_procs.consume(id, [&result](value_type const& lvl) {
        result = lvl;
    });
    return result;
}

template auto audio_engine::get_parameter_update(bool_parameter_id) const
    -> std::optional<bool>;

template auto audio_engine::get_parameter_update(float_parameter_id) const
    -> std::optional<float>;

template auto audio_engine::get_parameter_update(int_parameter_id) const
    -> std::optional<int>;

template auto audio_engine::get_parameter_update(enum_parameter_id) const
    -> std::optional<int>;

auto
audio_engine::get_learned_midi() const -> std::optional<midi::external_event>
{
    std::optional<midi::external_event> result;

    if (m_impl->midi_learn_output_proc)
    {
        m_impl->midi_learn_output_proc->consume(
            [&result](midi::external_event const& ev) { result = ev; });
    }

    return result;
}

auto
audio_engine::get_stream(audio_stream_id const id) const -> audio_stream_buffer
{
    if (auto proc = m_impl->stream_procs.find_processor(id))
    {
        return audio_stream_buffer{proc->consume()};
    }

    return audio_stream_buffer{};
}

bool
audio_engine::rebuild(
    state const& st,
    fx::simple_ladspa_processor_factory const& ladspa_fx_proc_factory,
    std::unique_ptr<midi::input_event_handler> midi_in)
{
    component_map comps;
    processor_map procs;

    make_mixer_components(
        comps,
        m_impl->comps,
        m_impl->sample_rate,
        st.strings,
        st.mixer_state,
        st.params,
        m_impl->param_procs,
        m_impl->stream_procs);
    make_fx_chain_components(
        comps,
        m_impl->comps,
        m_impl->sample_rate,
        st.fx_state,
        st.params,
        ladspa_fx_proc_factory,
        m_impl->param_procs,
        m_impl->stream_procs);
    auto const solo_groups = runtime::solo_groups(
        st.mixer_state.channels,
        st.mixer_state.io_map,
        st.mixer_state.aux_sends,
        st.params);
    make_solo_group_components(comps, solo_groups, m_impl->param_procs);

    bool const midi_learn = static_cast<bool>(st.midi_learning);
    make_midi_processors(std::move(midi_in), midi_learn, procs);
    auto disabled_assignments = mixer::disabled_by_routing_aux_sends(
        st.mixer_state.io_map,
        st.mixer_state.aux_sends,
        st.params);
    make_midi_assignment_processors(
        st.midi_assignments,
        disabled_assignments,
        st.params,
        m_impl->param_procs,
        procs,
        m_impl->procs);
    auto midi_learn_output_proc =
        midi_learn
            ? std::make_unique<
                  audio::engine::value_io_processor<midi::external_event>>(
                  "midi_learned")
            : nullptr;

    auto new_graph = make_graph(
        comps,
        st.mixer_state,
        st.external_audio_state,
        st.params,
        m_impl->input_procs,
        m_impl->output_procs);

    connect_midi(
        new_graph,
        procs,
        midi_learn_output_proc.get(),
        st.midi_assignments,
        disabled_assignments);

    connect_solo_groups(new_graph, comps, solo_groups);

    auto [final_graph, mixers] = audio::engine::finalize_graph(new_graph);

    m_impl->param_procs.initialize([&st](auto const id) {
        auto const* const slot = st.params.find(id);
        return slot ? std::optional{slot->get()} : std::nullopt;
    });

    if (!m_impl->process.swap_executor(
            audio::engine::graph_to_dag(final_graph)
                .make_runnable(m_impl->worker_threads)))
    {
        return false;
    }

    m_impl->graph = std::move(final_graph);
    m_impl->mixer_procs = std::move(mixers);
    m_impl->midi_learn_output_proc = std::move(midi_learn_output_proc);
    m_impl->procs = std::move(procs);
    m_impl->comps = std::move(comps);

    m_impl->param_procs.clear_expired();
    m_impl->stream_procs.clear_expired();

    {
        std::ofstream os("graph.dot");
        audio::engine::export_graph_as_dot(new_graph, os) << std::endl;
    }

    {
        std::ofstream os("final_graph.dot");
        audio::engine::export_graph_as_dot(m_impl->graph, os) << std::endl;
    }

    return true;
}

void
audio_engine::init_process(
    std::span<audio::pcm_input_buffer_converter const> const in_conv,
    std::span<audio::pcm_output_buffer_converter const> const out_conv)
{
    BOOST_ASSERT(m_impl->input_procs.size() == in_conv.size());
    for (std::size_t const i : range::indices(in_conv))
    {
        m_impl->input_procs[i]->set_input(in_conv[i]);
    }

    BOOST_ASSERT(m_impl->output_procs.size() == out_conv.size());
    for (std::size_t const i : range::indices(out_conv))
    {
        m_impl->output_procs[i]->set_output(out_conv[i]);
    }
}

auto
audio_engine::process(std::size_t const buffer_size) noexcept
    -> std::chrono::nanoseconds
{
    return m_impl->process(buffer_size);
}

} // namespace piejam::runtime
