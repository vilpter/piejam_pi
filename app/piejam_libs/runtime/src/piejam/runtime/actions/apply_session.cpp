// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/actions/apply_session.h>

#include <piejam/functional/operators.h>
#include <piejam/range/indices.h>
#include <piejam/runtime/parameter/assignment.h>
#include <piejam/runtime/persistence/session.h>
#include <piejam/runtime/state.h>
#include <piejam/set_if.h>

#include <boost/assert.hpp>
#include <boost/hof/match.hpp>

namespace piejam::runtime::actions
{

namespace
{

template <io_direction D>
auto
apply_external_audio_device_configs(
    state& st,
    std::span<persistence::session::external_audio_device_config const> configs)
{
    BOOST_ASSERT_MSG(
        st.external_audio_state.io_ids[D]->empty(),
        "configs should be cleared before applying");

    for (auto const& config : configs)
    {
        auto const type = D == io_direction::output ? audio::bus_type::stereo
                                                    : config.bus_type;

        auto device_id =
            runtime::add_external_audio_device(st, config.name, D, type);

        std::visit(
            boost::hof::match(
                [&](std::size_t const mono_channel) {
                    if (type == audio::bus_type::mono)
                    {
                        st.external_audio_state.device_channels.assign(
                            {device_id, audio::bus_channel::mono},
                            mono_channel);
                    }
                },
                [&](audio::pair<std::size_t> const& stereo_channels) {
                    if (type == audio::bus_type::stereo)
                    {
                        st.external_audio_state.device_channels.assign(
                            {device_id, audio::bus_channel::left},
                            stereo_channels.left);
                        st.external_audio_state.device_channels.assign(
                            {device_id, audio::bus_channel::right},
                            stereo_channels.right);
                    }
                }),
            config.assigned_channels);
    }
}

void
apply_mixer_fx_chain(
    state& st,
    mixer::channel_id const channel_id,
    persistence::session::fx_chain_t const& fx_chain_data)
{
    for (auto const& fx_plug : fx_chain_data)
    {
        auto fx_mod_id = std::visit(
            boost::hof::match(
                [&](persistence::session::internal_fx const& fx) {
                    return runtime::insert_internal_fx_module(
                        st,
                        channel_id,
                        npos,
                        fx.type,
                        fx.preset,
                        fx.midi);
                },
                [&](persistence::session::ladspa_plugin const& ladspa_plug) {
                    return runtime::insert_missing_ladspa_fx_module(
                        st,
                        channel_id,
                        npos,
                        fx::unavailable_ladspa{
                            .plugin_id = ladspa_plug.id,
                            .parameter_values = ladspa_plug.preset,
                            .midi_assignments = ladspa_plug.midi},
                        ladspa_plug.name);
                },
                [](auto const&) -> fx::module_id {
                    BOOST_ASSERT(false);
                    return fx::module_id{};
                }),
            fx_plug.as_variant());

        st.params.at(st.fx_state.active_modules.at(fx_mod_id))
            .set(fx_plug.active);
    }
}

auto
find_mixer_channel(mixer::state const& mixer_state, std::size_t const& index)
    -> std::optional<mixer::channel_id>
{
    if (index == 0)
    {
        return {mixer_state.main};
    }
    else if ((index - 1) < mixer_state.inputs->size())
    {
        return {(*mixer_state.inputs)[index - 1]};
    }
    else
    {
        return std::nullopt;
    }
}

auto
find_external_audio_device_route(
    external_audio::device_ids_t const& devices,
    std::size_t const index)
{
    return index < devices.size() ? mixer::io_address_t{devices[index]}
                                  : mixer::io_address_t{};
}

void
apply_mixer_io(
    parameter::store& params,
    external_audio::state& external_audio_state,
    mixer::state& mixer_state,
    midi_assignments_map& midi_assigns_store,
    mixer::channel_id const& channel_id,
    persistence::session::mixer_io const& in,
    persistence::session::mixer_io const& out,
    std::vector<persistence::session::mixer_aux_send> const& aux_sends_data)
{
    auto find_route = [&](auto const& mixer_io, io_direction io_dir) {
        switch (mixer_io.type)
        {
            case persistence::session::mixer_io_type::device:
                return find_external_audio_device_route(
                    external_audio_state.io_ids[io_dir],
                    mixer_io.index);

            case persistence::session::mixer_io_type::channel:
                if (auto found_channel_id =
                        find_mixer_channel(mixer_state, mixer_io.index))
                {
                    return mixer::io_address_t{*found_channel_id};
                }

                return mixer::io_address_t{};

            case persistence::session::mixer_io_type::mix:
                return mixer::io_address_t{mixer::mix_input{}};

            default:
                return mixer::io_address_t();
        }
    };

    mixer_state.io_map.lock().at(channel_id) = {
        find_route(in, io_direction::input),
        find_route(out, io_direction::output)};

    if (auto channel_aux_sends = mixer_state.aux_sends.find(channel_id))
    {
        for (auto const& aux_send_data : aux_sends_data)
        {
            if (auto route = find_mixer_channel(
                    mixer_state,
                    aux_send_data.channel_index))
            {
                if (auto aux_send = channel_aux_sends->find(*route))
                {
                    apply_parameter_values(
                        aux_send_data.parameter,
                        aux_send->parameters,
                        params);

                    apply_midi_assignments(
                        aux_send_data.midi,
                        aux_send->parameters,
                        midi_assigns_store);
                }
            }
        }
    }
}

void
apply_aux_channels(
    parameter::store& params,
    mixer::state& mixer_state,
    std::vector<persistence::session::aux_channel> const& aux_channels_data)
{
    for (auto const& aux_channel_data : aux_channels_data)
    {
        if (auto aux_id =
                find_mixer_channel(mixer_state, aux_channel_data.channel_index);
            aux_id)
        {
            if (auto aux_channel = mixer_state.aux_channels.find(*aux_id);
                aux_channel)
            {
                apply_parameter_values(
                    aux_channel_data.parameter,
                    aux_channel->parameters,
                    params);
            }
        }
    }
}

} // namespace

void
apply_session::reduce(state& st) const
{
    reset_state(st);

    apply_external_audio_device_configs<io_direction::input>(
        st,
        session->external_audio_input_devices);

    apply_external_audio_device_configs<io_direction::output>(
        st,
        session->external_audio_output_devices);

    BOOST_ASSERT(st.mixer_state.inputs->empty());
    BOOST_ASSERT(st.mixer_state.channels.size() == 1);
    BOOST_ASSERT(st.mixer_state.channels.find(st.mixer_state.main));

    midi_assignments_map mixer_midi_assignments;
    for (auto const& channel_data : session->mixer_channels)
    {
        auto added_channel_id = runtime::add_mixer_channel(
            st,
            channel_data.channel_type,
            channel_data.name);
        auto& added_channel = st.mixer_state.channels.at(added_channel_id);
        st.material_colors.assign(added_channel.color, channel_data.color);
        apply_midi_assignments(
            channel_data.midi,
            *added_channel.parameters,
            mixer_midi_assignments);
        apply_parameter_values(
            channel_data.parameter,
            *added_channel.parameters,
            st.params);
        apply_mixer_fx_chain(st, added_channel_id, channel_data.fx_chain);
    }

    auto const& main_mixer_channel =
        st.mixer_state.channels.at(st.mixer_state.main);

    st.strings.assign(
        main_mixer_channel.name,
        box{session->main_mixer_channel.name});
    st.material_colors.assign(
        main_mixer_channel.color,
        session->main_mixer_channel.color);
    apply_midi_assignments(
        session->main_mixer_channel.midi,
        *main_mixer_channel.parameters,
        mixer_midi_assignments);
    apply_parameter_values(
        session->main_mixer_channel.parameter,
        *main_mixer_channel.parameters,
        st.params);

    apply_mixer_fx_chain(
        st,
        st.mixer_state.main,
        session->main_mixer_channel.fx_chain);

    apply_aux_channels(st.params, st.mixer_state, session->aux_channels);

    // mixer I/O can only be applied after all channels are created
    for (std::size_t channel_index : range::indices(session->mixer_channels))
    {
        auto channel_id = (*st.mixer_state.inputs)[channel_index];
        auto const& channel_data = session->mixer_channels[channel_index];

        apply_mixer_io(
            st.params,
            st.external_audio_state,
            st.mixer_state,
            mixer_midi_assignments,
            channel_id,
            channel_data.in,
            channel_data.out,
            channel_data.aux_sends);
    }

    apply_mixer_io(
        st.params,
        st.external_audio_state,
        st.mixer_state,
        mixer_midi_assignments,
        st.mixer_state.main,
        session->main_mixer_channel.in,
        session->main_mixer_channel.out,
        session->main_mixer_channel.aux_sends);

    runtime::update_midi_assignments(st, mixer_midi_assignments);

    st.current_session = current_session;
    st.session_modified = false;
}

} // namespace piejam::runtime::actions
