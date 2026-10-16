// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/state.h>

#include <piejam/runtime/enum_parameter.h>
#include <piejam/runtime/fader_mapping.h>
#include <piejam/runtime/internal_fx_module_factory.h>
#include <piejam/runtime/ladspa_fx/ladspa_fx_module.h>
#include <piejam/runtime/parameter_factory.h>

#include <piejam/algorithm/erase_if.h>
#include <piejam/audio/types.h>
#include <piejam/functional/operators.h>
#include <piejam/indexed_access.h>
#include <piejam/ladspa/port_descriptor.h>
#include <piejam/numeric/dB_convert.h>
#include <piejam/set_if.h>
#include <piejam/tuple_element_compare.h>

#include <boost/hof/match.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/range/algorithm_ext/erase.hpp>

#include <algorithm>
#include <format>
#include <ranges>

namespace piejam::runtime
{

namespace
{

constexpr auto
to_normalized_volume(float_parameter const&, float const value)
{
    return fader_mapping::to_normalized_dB_mapping<fader_mapping::volume>(
        value);
}

constexpr auto
from_normalized_volume(float_parameter const&, float const norm_value) -> float
{
    return fader_mapping::from_normalized_dB_mapping<fader_mapping::volume>(
        norm_value);
}

constexpr auto
to_normalized_send(float_parameter const&, float const value)
{
    return fader_mapping::to_normalized_dB_mapping<fader_mapping::send>(value);
}

constexpr auto
from_normalized_send(float_parameter const&, float const norm_value) -> float
{
    return fader_mapping::from_normalized_dB_mapping<fader_mapping::send>(
        norm_value);
}

auto
volume_to_string(float volume) -> std::string
{
    auto const volume_dB = numeric::to_dB(volume, 1.e-20f);
    return std::format("{:.1f} dB", volume_dB);
}

void
reset_to_default(parameter::store& params, parameters_map const& map)
{
    for (auto [key, id] : map)
    {
        std::visit(
            [&]<class P>(parameter::id_t<P> id) {
                auto& slot = params.at(id);
                slot.set(slot.param().default_value);
            },
            id);
    }
}

} // namespace

auto
make_initial_state() -> state
{
    state st;
    st.mixer_state.main =
        add_mixer_channel(st, mixer::channel_type::stereo, "Main", true);

    // main doesn't belong into inputs
    remove_erase(st.mixer_state.inputs, st.mixer_state.main);

    // reset io
    st.mixer_state.io_map.assign(
        st.mixer_state.main,
        io_pair<mixer::io_address_t>{mixer::mix_input{}, default_t{}});

    return st;
}

void
apply_parameter_values(
    std::span<parameter_value_assignment const> values,
    parameters_map const& parameters,
    parameter::store& params_store)
{
    for (auto&& [key, value] : values)
    {
        if (auto param_id = parameters.find(key); param_id)
        {
            std::visit(
                boost::hof::match(
                    [&params_store]<class P>(
                        parameter::id_t<P> id,
                        parameter::tagged_value<P> v) {
                        params_store.at(id).set(v);
                    },
                    [](auto&&, auto&&) { BOOST_ASSERT(false); }),
                *param_id,
                value);
        }
    }
}

static void
update_midi_assignments(
    midi_assignments_map& midi_assigns,
    midi_assignments_map const& new_assignments)
{
    for (auto&& [id, ass] : new_assignments)
    {
        algorithm::erase_if(
            midi_assigns,
            tuple::element<1>.equal_to(std::cref(ass)));

        midi_assigns.insert_or_assign(id, ass);
    }
}

void
apply_midi_assignments(
    std::span<parameter_midi_assignment const> midi_assigns,
    parameters_map const& parameters,
    midi_assignments_map& midi_assigns_store)
{
    midi_assignments_map new_assignments;
    for (auto&& [key, value] : midi_assigns)
    {
        if (auto param_id = parameters.find(key); param_id)
        {
            new_assignments.emplace(*param_id, value);
        }
    }

    update_midi_assignments(midi_assigns_store, new_assignments);
}

static auto
insert_fx_module(
    state& st,
    mixer::channel_id const mixer_channel_id,
    std::size_t const position,
    fx::module_id fx_mod_id,
    std::span<parameter_value_assignment const> initial_values,
    std::span<parameter_midi_assignment const> midi_assigns)
{
    fx::chain_t fx_chain = st.mixer_state.fx_chains.at(mixer_channel_id);
    auto const insert_pos = std::min(position, fx_chain.size());

    fx_chain.emplace(std::next(fx_chain.begin(), insert_pos), fx_mod_id);

    auto const& fx_mod = st.fx_state.modules.at(fx_chain[insert_pos]);

    apply_parameter_values(initial_values, fx_mod.parameters, st.params);
    apply_midi_assignments(
        midi_assigns,
        fx_mod.parameters,
        *st.midi_assignments.lock());

    st.mixer_state.fx_chains.assign(mixer_channel_id, std::move(fx_chain));

    parameter_factory params{st.params};
    st.fx_state.active_modules.emplace(
        fx_mod_id,
        params.make_parameter(
            make_bool_parameter({
                                    .name = "Active",
                                    .default_value = true,
                                })
                .set_flags(
                    {parameter_flags::not_midi_assignable,
                     parameter_flags::audio_graph_affecting})));

    st.session_modified = true;
}

auto
insert_internal_fx_module(
    state& st,
    mixer::channel_id const channel_id,
    std::size_t const position,
    fx::internal_id fx_internal_id,
    std::span<parameter_value_assignment const> initial_values,
    std::span<parameter_midi_assignment const> midi_assigns) -> fx::module_id
{
    fx::module_id fx_mod_id = st.fx_state.modules.emplace(
        internal_fx_module_factories::lookup(fx_internal_id)({
            .bus_type =
                to_bus_type(st.mixer_state.channels.at(channel_id).type),
            .sample_rate = st.sample_rate,
            .params = st.params,
            .streams = st.streams,
        }));

    insert_fx_module(
        st,
        channel_id,
        position,
        fx_mod_id,
        initial_values,
        midi_assigns);

    return fx_mod_id;
}

auto
insert_ladspa_fx_module(
    state& st,
    mixer::channel_id const channel_id,
    std::size_t const position,
    ladspa::instance_id const instance_id,
    ladspa::plugin_descriptor const& plugin_desc,
    std::span<ladspa::port_descriptor const> const control_inputs,
    std::span<parameter_value_assignment const> initial_values,
    std::span<parameter_midi_assignment const> midi_assigns) -> fx::module_id
{
    auto fx_mod_id = st.fx_state.modules.emplace(
        ladspa_fx::make_module(
            instance_id,
            plugin_desc.name,
            to_bus_type(st.mixer_state.channels.at(channel_id).type),
            control_inputs,
            st.params));
    st.fx_state.ladspa_instances.emplace(instance_id, plugin_desc);

    insert_fx_module(
        st,
        channel_id,
        position,
        fx_mod_id,
        initial_values,
        midi_assigns);

    return fx_mod_id;
}

auto
insert_missing_ladspa_fx_module(
    state& st,
    mixer::channel_id const channel_id,
    std::size_t const position,
    fx::unavailable_ladspa const& unavail,
    std::string_view const name) -> fx::module_id
{
    auto id = st.fx_state.unavailable_ladspa_plugins.emplace(unavail);
    auto fx_mod_id = st.fx_state.modules.emplace(
        fx::module{
            .fx_instance_id = id,
            .name = box(std::string(name)),
            .bus_type =
                to_bus_type(st.mixer_state.channels.at(channel_id).type),
            .parameters = {},
            .streams = {},
        });

    insert_fx_module(st, channel_id, position, fx_mod_id, {}, {});

    return fx_mod_id;
}

template <class P>
static auto
remove_midi_assignement_for_parameter(state& st, parameter::id_t<P> id)
{
    st.midi_assignments.lock()->erase(id);
}

template <class P>
static auto
remove_parameter(state& st, parameter::id_t<P> id)
{
    st.params.remove(id);

    remove_midi_assignement_for_parameter(st, id);
}

static auto
remove_parameters(state& st, parameters_map const& params)
{
    for (auto&& [key, param_id] : params)
    {
        std::visit([&st](auto&& id) { remove_parameter(st, id); }, param_id);
    }
}

void
remove_fx_module(
    state& st,
    mixer::channel_id const fx_chain_id,
    fx::module_id const fx_mod_id)
{
    fx::module const& fx_mod = st.fx_state.modules.at(fx_mod_id);

    fx::chain_t fx_chain = st.mixer_state.fx_chains.at(fx_chain_id);
    BOOST_ASSERT(std::ranges::contains(fx_chain, fx_mod_id));
    boost::remove_erase(fx_chain, fx_mod_id);
    st.mixer_state.fx_chains.assign(fx_chain_id, std::move(fx_chain));

    remove_parameters(st, *fx_mod.parameters);

    if (auto id = std::get_if<ladspa::instance_id>(&fx_mod.fx_instance_id))
    {
        st.fx_state.ladspa_instances.erase(*id);
    }
    else if (
        auto id =
            std::get_if<fx::unavailable_ladspa_id>(&fx_mod.fx_instance_id))
    {
        st.fx_state.unavailable_ladspa_plugins.erase(*id);
    }

    remove_parameter(st, st.fx_state.active_modules.at(fx_mod_id));
    st.fx_state.active_modules.erase(fx_mod_id);

    st.fx_state.modules.erase(fx_mod_id);

    st.session_modified = true;
}

static auto
make_aux_send(parameter_factory const& params_factory)
{
    using namespace std::string_literals;
    return mixer::aux_send{
        .parameters = box{parameters_map{
            std::in_place_type<mixer::aux_send::parameter_key>,
            {
                {mixer::aux_send::parameter_key::active,
                 params_factory.make_parameter(
                     make_bool_parameter({.name = "Active"})
                         .set_flags({parameter_flags::audio_graph_affecting}))},
                {mixer::aux_send::parameter_key::fader_tap,
                 params_factory.make_parameter(
                     make_enum_parameter(
                         "Fader Tap",
                         mixer::aux_send_fader_tap::auto_,
                         &mixer::aux_send::to_fader_tap_string)
                         .set_flags(
                             {parameter_flags::not_midi_assignable,
                              parameter_flags::audio_graph_affecting}))},
                {mixer::aux_send::parameter_key::volume,
                 params_factory.make_parameter(
                     make_float_parameter(
                         {
                             .name = "Send"s,
                             .default_value = 0.f,
                         },
                         {
                             .min = 0.f,
                             .max = 1.f,
                             .to_normalized = &to_normalized_send,
                             .from_normalized = &from_normalized_send,
                         })
                         .set_value_to_string(&volume_to_string))},
            }}}};
}

static auto
make_aux_channel(parameter_factory& params)
{
    return mixer::aux_channel{
        .parameters = box{parameters_map{
            std::in_place_type<mixer::aux_channel::parameter_key>,
            {
                {mixer::aux_channel::parameter_key::default_fader_tap,
                 params.make_parameter(
                     make_enum_parameter(
                         "Fader Tap",
                         mixer::aux_channel_fader_tap::post,
                         &mixer::aux_channel::to_fader_tap_string)
                         .set_flags(
                             {parameter_flags::not_midi_assignable,
                              parameter_flags::audio_graph_affecting}))},
            }}}};
}

static auto
remove_aux_send(state& st, mixer::aux_send const& aux_send)
{
    remove_parameters(st, aux_send.parameters);
}

static auto
make_mixer_channel(
    state& st,
    mixer::channel_type type,
    std::string name,
    bool default_record)
{
    using namespace std::string_view_literals;

    auto name_id = string_id::generate();
    st.strings.insert(name_id, box{std::move(name)});

    auto color_id = material_color_id::generate();
    st.material_colors.insert(color_id, material_color::pink);

    parameter_factory params_factory{st.params};

    return st.mixer_state.channels.emplace(
        mixer::channel{
            .type = type,
            .name = name_id,
            .color = color_id,
            .parameters = box{parameters_map{
                std::in_place_type<mixer::channel::parameter_key>,
                {
                    {mixer::channel::parameter_key::volume,
                     params_factory.make_parameter(
                         make_float_parameter(
                             {
                                 .name = "Volume"sv,
                                 .default_value = 1.f,
                             },
                             {
                                 .min = 0.f,
                                 .max = numeric::from_dB(6.f),
                                 .to_normalized = &to_normalized_volume,
                                 .from_normalized = &from_normalized_volume,
                             })
                             .set_value_to_string(&volume_to_string))},
                    {mixer::channel::parameter_key::pan_balance,
                     params_factory.make_parameter(
                         make_float_parameter(
                             {
                                 .name = bool_enum_to(
                                     to_bus_type(type),
                                     "Pan"sv,
                                     "Balance"sv),
                                 .default_value = 0.f,
                             },
                             linear_float_parameter_range<-1.f, 1.f>())
                             .set_flags({parameter_flags::bipolar}))},
                    {mixer::channel::parameter_key::record,
                     params_factory.make_parameter(make_bool_parameter({
                         .name = "Record"sv,
                         .default_value = default_record,
                     }))},
                    {mixer::channel::parameter_key::mute,
                     params_factory.make_parameter(make_bool_parameter({
                         .name = "Mute"sv,
                     }))},
                    {mixer::channel::parameter_key::solo,
                     params_factory.make_parameter(
                         make_bool_parameter({
                                                 .name = "Solo"sv,
                                             })
                             .set_flags(
                                 {parameter_flags::solo_state_affecting}))},
                }}},
            .out_stream = make_stream(st.streams, 2),
        });
}

auto
add_external_audio_device(
    state& st,
    std::string name,
    io_direction const io_dir,
    audio::bus_type const bus_type) -> external_audio::device_id
{
    auto name_id = string_id::generate();
    st.strings.insert(name_id, box{std::move(name)});

    BOOST_ASSERT(
        io_dir == io_direction::input || bus_type == audio::bus_type::stereo);

    auto id = st.external_audio_state.devices.emplace(
        external_audio::device{
            .name = name_id,
            .bus_type = bus_type,
        });

    emplace_back(st.external_audio_state.io_ids[io_dir], id);

    if (bus_type == audio::bus_type::mono)
    {
        st.external_audio_state.device_channels.emplace(
            external_audio::device_channel_key{id, audio::bus_channel::mono},
            npos);
    }
    else
    {
        st.external_audio_state.device_channels.emplace(
            external_audio::device_channel_key{id, audio::bus_channel::left},
            npos);
        st.external_audio_state.device_channels.emplace(
            external_audio::device_channel_key{id, audio::bus_channel::right},
            npos);
    }

    st.session_modified = true;

    return id;
}

auto
add_mixer_channel(
    state& st,
    mixer::channel_type type,
    std::string name,
    bool default_record) -> mixer::channel_id
{
    auto channel_id =
        make_mixer_channel(st, type, std::move(name), default_record);
    emplace_back(st.mixer_state.inputs, channel_id);

    st.mixer_state.io_map.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(channel_id),
        std::forward_as_tuple(
            type == mixer::channel_type::aux
                ? mixer::io_address_t{mixer::mix_input{}}
                : mixer::io_address_t{},
            st.mixer_state.main));

    parameter_factory params_factory{st.params};

    if (type == mixer::channel_type::aux)
    {
        st.mixer_state.aux_channels.emplace(
            channel_id,
            make_aux_channel(params_factory));

        // add as aux_send to each channel
        [&](auto&& aux_sends) {
            for (auto&& [ch_id, ch_aux_sends] : aux_sends)
            {
                ch_aux_sends.emplace(channel_id, make_aux_send(params_factory));
            }
        }(st.mixer_state.aux_sends.lock());
    }
    else
    {
        st.mixer_state.aux_sends.emplace(
            channel_id,
            mixer::channel_aux_sends_t{});
    }

    st.mixer_state.fx_chains.emplace(channel_id, fx::chain_t{});

    st.session_modified = true;

    return channel_id;
}

static void
reset_io_targets(mixer::io_map_t& io_map, mixer::io_address_t target)
{
    auto io_map_locked = io_map.lock();
    for (auto& [id, io] : io_map_locked)
    {
        for (auto& addr : io)
        {
            if (addr == target)
            {
                addr = default_t{};
            }
        }
    }
}

void
remove_mixer_channel(state& st, mixer::channel_id const mixer_channel_id)
{
    BOOST_ASSERT(mixer_channel_id != st.mixer_state.main);

    mixer::channel const& mixer_channel =
        st.mixer_state.channels.at(mixer_channel_id);

    st.strings.erase(mixer_channel.name);
    st.material_colors.erase(mixer_channel.color);

    remove_parameters(st, mixer_channel.parameters);

    [&](auto&& aux_sends) {
        if (mixer_channel.type == mixer::channel_type::aux)
        {
            // remove itself as aux_send from other channels
            for (auto& [ch_id, ch_aux_sends] : aux_sends)
            {
                remove_aux_send(st, ch_aux_sends.at(mixer_channel_id));
                ch_aux_sends.erase(mixer_channel_id);
            }

            st.mixer_state.aux_channels.erase(mixer_channel_id);
        }
        else
        {
            // remove own aux_sends
            for (auto const& [aux_id, aux_send] :
                 aux_sends.at(mixer_channel_id))
            {
                remove_aux_send(st, aux_send);
            }

            aux_sends.erase(mixer_channel_id);
        }
    }(st.mixer_state.aux_sends.lock());

    for (auto fx_mod_id :
         std::views::reverse(st.mixer_state.fx_chains.at(mixer_channel_id)))
    {
        remove_fx_module(st, mixer_channel_id, fx_mod_id);
    }

    if (st.focused_fx_chain_id == mixer_channel_id)
    {
        st.focused_fx_chain_id = {};
        st.focused_fx_mod_id = {};
    }

    if (st.fx_browser_fx_chain_id == mixer_channel_id)
    {
        st.fx_browser_fx_chain_id = mixer::channel_id{};
    }

    BOOST_ASSERT(
        std::ranges::contains(*st.mixer_state.inputs, mixer_channel_id));
    remove_erase(st.mixer_state.inputs, mixer_channel_id);

    st.streams.erase(mixer_channel.out_stream);

    reset_io_targets(st.mixer_state.io_map, mixer_channel_id);
    st.mixer_state.io_map.erase(mixer_channel_id);

    st.mixer_state.fx_chains.erase(mixer_channel_id);

    st.mixer_state.channels.erase(mixer_channel_id);

    st.session_modified = true;
}

void
remove_external_audio_device(
    state& st,
    external_audio::device_id const device_id)
{
    auto [name_id, bus_type] = st.external_audio_state.devices.at(device_id);

    st.strings.erase(name_id);

    if (bus_type == audio::bus_type::mono)
    {
        st.external_audio_state.device_channels.erase(
            external_audio::device_channel_key{
                device_id,
                audio::bus_channel::mono});
    }
    else
    {
        st.external_audio_state.device_channels.erase(
            external_audio::device_channel_key{
                device_id,
                audio::bus_channel::left});
        st.external_audio_state.device_channels.erase(
            external_audio::device_channel_key{
                device_id,
                audio::bus_channel::right});
    }

    reset_io_targets(st.mixer_state.io_map, device_id);

    if (std::ranges::contains(*st.external_audio_state.io_ids.in(), device_id))
    {
        remove_erase(st.external_audio_state.io_ids.in(), device_id);
    }
    else
    {
        BOOST_ASSERT(
            std::ranges::contains(
                *st.external_audio_state.io_ids.out(),
                device_id));
        remove_erase(st.external_audio_state.io_ids.out(), device_id);
    }

    st.external_audio_state.devices.erase(device_id);

    st.session_modified = true;
}

void
update_midi_assignments(state& st, midi_assignments_map const& assignments)
{
    update_midi_assignments(*st.midi_assignments.lock(), assignments);

    st.session_modified = true;
}

void
reset_state(state& st)
{
    using namespace std::string_literals;

    auto inputs = st.mixer_state.inputs;
    for (auto input_id : inputs.get())
    {
        remove_mixer_channel(st, input_id);
    }

    auto main_fx_chain = st.mixer_state.fx_chains.at(st.mixer_state.main);
    for (auto fx_mod_id : main_fx_chain)
    {
        remove_fx_module(st, st.mixer_state.main, fx_mod_id);
    }

    for (auto const& [device_id, device] : st.external_audio_state.devices)
    {
        remove_external_audio_device(st, device_id);
    }

    // reset main channel
    auto const& main_channel = st.mixer_state.channels.at(st.mixer_state.main);
    st.strings.assign(main_channel.name, box{"Main"s});
    st.material_colors.assign(main_channel.color, material_color::pink);
    reset_to_default(st.params, main_channel.parameters);
    st.mixer_state.io_map.assign(
        st.mixer_state.main,
        io_pair<mixer::io_address_t>{mixer::mix_input{}, default_t{}});

    st.midi_assignments.lock()->clear();

    st.recording = false;
    st.rec_session += 1;
    st.rec_take = 0;

    st.current_session = {};
    st.session_modified = false;

    BOOST_ASSERT(st.strings.size() == 1);
    BOOST_ASSERT(st.material_colors.size() == 1);
    BOOST_ASSERT(st.external_audio_state.devices.empty());
    BOOST_ASSERT(st.mixer_state.inputs->empty());
    BOOST_ASSERT(st.mixer_state.channels.size() == 1);
    BOOST_ASSERT(st.midi_assignments->empty());
}

} // namespace piejam::runtime
