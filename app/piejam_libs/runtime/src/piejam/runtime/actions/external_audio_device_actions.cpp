// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/actions/external_audio_device_actions.h>

#include <piejam/runtime/state.h>

#include <piejam/functional/operators.h>
#include <piejam/set_if.h>

#include <boost/assert.hpp>

#include <algorithm>
#include <format>

namespace piejam::runtime::actions
{

namespace
{

auto
default_bus_name(state const& st, io_direction io_dir, audio::bus_type bus_type)
    -> std::string
{
    using namespace std::string_literals;

    switch (io_dir)
    {
        case io_direction::input:
            return std::format(
                "In {} {}",
                st.external_audio_state.io_ids.in()->size() + 1,
                bus_type == audio::bus_type::mono ? "M" : "S");

        case io_direction::output:
            switch (st.external_audio_state.io_ids.out()->size())
            {
                case 0:
                    return "Speaker"s;

                case 1:
                    return "Cue"s;

                default:
                    return std::format(
                        "Aux {}",
                        st.external_audio_state.io_ids.out()->size() - 1);
            }
    }
}

auto
assigned_channels(state const& st, io_direction io_dir)
{
    auto num_channels = st.selected_sound_card.num_channels[io_dir];
    std::vector<bool> assigned_channels(num_channels);

    auto const& device_ids = *st.external_audio_state.io_ids[io_dir];

    auto mark_assigned = [&](auto const device_id, auto const bus_channel) {
        if (auto ch = st.external_audio_state.device_channels.at(
                {device_id, bus_channel});
            ch < assigned_channels.size())
        {
            assigned_channels[ch] = true;
        }
    };

    for (external_audio::device_id const device_id : device_ids)
    {
        auto const bus_type =
            st.external_audio_state.devices.at(device_id).bus_type;

        if (bus_type == audio::bus_type::mono)
        {
            mark_assigned(device_id, audio::bus_channel::mono);
        }
        else
        {
            mark_assigned(device_id, audio::bus_channel::left);
            mark_assigned(device_id, audio::bus_channel::right);
        }
    }

    return assigned_channels;
}

auto
find_unassigned_channel(std::vector<bool> const& assigned_channels)
    -> std::size_t
{
    auto it = std::ranges::find(assigned_channels, false);
    return it != assigned_channels.end()
               ? std::distance(assigned_channels.begin(), it)
               : npos;
}

} // namespace

void
add_external_audio_device::reduce(state& st) const
{
    BOOST_ASSERT(
        direction != io_direction::output || type == audio::bus_type::stereo);

    auto added_device_id = runtime::add_external_audio_device(
        st,
        default_bus_name(st, direction, type),
        direction,
        type);

    auto channels = assigned_channels(st, direction);

    auto find_and_assign_channel = [&](audio::bus_channel bus_ch) {
        auto ch = find_unassigned_channel(channels);
        if (ch != npos)
        {
            st.external_audio_state.device_channels.assign(
                {added_device_id, bus_ch},
                ch);
            channels[ch] = true;
        }
    };

    if (type == audio::bus_type::mono)
    {
        find_and_assign_channel(audio::bus_channel::mono);
    }
    else
    {
        find_and_assign_channel(audio::bus_channel::left);
        find_and_assign_channel(audio::bus_channel::right);
    }

    if (direction == io_direction::output)
    {
        if (std::holds_alternative<default_t>(
                st.mixer_state.io_map.at(st.mixer_state.main).out()))
        {
            st.mixer_state.io_map.lock().at(st.mixer_state.main).out() =
                added_device_id;
        }
    }
}

void
remove_external_audio_device::reduce(state& st) const
{
    runtime::remove_external_audio_device(st, device_id);
}

void
set_external_audio_device_bus_channel::reduce(state& st) const
{
    st.external_audio_state.device_channels.assign(
        {device_id, channel_selector},
        channel_index);

    st.session_modified = true;
}

} // namespace piejam::runtime::actions
