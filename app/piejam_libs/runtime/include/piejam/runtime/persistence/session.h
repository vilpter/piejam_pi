// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/fx/fwd.h>
#include <piejam/runtime/material_color.h>
#include <piejam/runtime/midi_assignment.h>
#include <piejam/runtime/mixer_fwd.h>
#include <piejam/runtime/parameter/assignment.h>
#include <piejam/runtime/persistence/parameter_midi_assignments.h>
#include <piejam/runtime/persistence/parameter_value_assignments.h>

#include <piejam/audio/pair.h>
#include <piejam/audio/types.h>
#include <piejam/ladspa/fwd.h>

#include <nlohmann/json_fwd.hpp>

#include <iosfwd>
#include <string>
#include <variant>
#include <vector>

namespace piejam::runtime::persistence
{

inline constexpr unsigned current_session_version = 0;

struct session
{
    struct external_audio_device_config
    {
        std::string name;
        audio::bus_type bus_type;
        external_audio::channels_config_t assigned_channels;
    };

    struct internal_fx
    {
        fx::internal_id type{};
        parameter_value_assignments preset;
        parameter_midi_assignments midi;
    };

    struct ladspa_plugin
    {
        ladspa::plugin_id_t id{};
        std::string name;
        parameter_value_assignments preset;
        parameter_midi_assignments midi;
    };

    struct fx_plugin : std::variant<std::monostate, internal_fx, ladspa_plugin>
    {
        using base_t = std::variant<std::monostate, internal_fx, ladspa_plugin>;
        using base_t::base_t;
        using base_t::operator=;

        [[nodiscard]]
        auto as_variant() const noexcept -> base_t const&
        {
            return *this;
        }

        bool active;
    };

    using fx_chain_t = std::vector<fx_plugin>;

    enum class mixer_io_type
    {
        none,
        mix,
        device,
        channel
    };

    struct mixer_io
    {
        mixer_io_type type;
        std::size_t index; // ch or external device
    };

    struct mixer_aux_send
    {
        std::size_t channel_index;
        parameter_value_assignments parameter;
        parameter_midi_assignments midi;
    };

    struct mixer_channel
    {
        std::string name;
        material_color color;
        mixer::channel_type channel_type;
        parameter_value_assignments parameter;
        parameter_midi_assignments midi;
        fx_chain_t fx_chain;
        mixer_io in;
        mixer_io out;
        std::vector<mixer_aux_send> aux_sends;
    };

    struct aux_channel
    {
        std::size_t channel_index;
        parameter_value_assignments parameter;
    };

    std::vector<external_audio_device_config> external_audio_input_devices;
    std::vector<external_audio_device_config> external_audio_output_devices;
    mixer_channel main_mixer_channel;
    std::vector<mixer_channel> mixer_channels;
    std::vector<aux_channel> aux_channels;
};

auto load_session(std::istream&) -> session;
void save_session(std::ostream&, session const&);

} // namespace piejam::runtime::persistence
