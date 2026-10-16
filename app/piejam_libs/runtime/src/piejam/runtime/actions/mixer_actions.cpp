// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/actions/mixer_actions.h>

#include <piejam/functional/get.h>
#include <piejam/runtime/state.h>

#include <boost/hof/compose.hpp>

#include <format>

namespace piejam::runtime::actions
{

namespace
{

auto
default_name(state const& st, mixer::channel_type channel_type) -> std::string
{
    switch (channel_type)
    {
        case mixer::channel_type::aux:
            return std::format(
                "Aux {}",
                st.mixer_state.aux_channels.size() + 1);

        default:
            return std::format(
                "In {}",
                st.mixer_state.inputs->size() -
                    st.mixer_state.aux_channels.size() + 1);
    }
}

} // namespace

void
add_mixer_channel::reduce(state& st) const
{
    auto added_mixer_channel_id = runtime::add_mixer_channel(
        st,
        channel_type,
        name.empty() ? default_name(st, channel_type) : name);

    // auto-assign to first available external audio device
    if (channel_type != mixer::channel_type::aux)
    {
        for (auto device_id : *st.external_audio_state.io_ids.in())
        {
            auto it = std::ranges::find(
                st.mixer_state.io_map,
                mixer::io_address_t{device_id},
                boost::hof::compose(get_by_index<0>, get_by_index<1>));

            if (it == st.mixer_state.io_map.end() &&
                st.external_audio_state.devices.at(device_id).bus_type ==
                    to_bus_type(channel_type))
            {
                st.mixer_state.io_map.lock().at(added_mixer_channel_id).in() =
                    device_id;
                break;
            }
        }
    }
}

void
delete_mixer_channel::reduce(state& st) const
{
    runtime::remove_mixer_channel(st, mixer_channel_id);
}

void
set_mixer_channel_color::reduce(state& st) const
{
    auto const& channel = st.mixer_state.channels.at(channel_id);
    st.material_colors.assign(channel.color, color);
}

void
set_mixer_channel_route::reduce(state& st) const
{
    BOOST_ASSERT_MSG(
        !(st.mixer_state.channels.at(channel_id).type ==
              mixer::channel_type::aux &&
          port == io_direction::input),
        "changing aux input is not allowed");
    st.mixer_state.io_map.lock().at(channel_id)[port] = route;
}

void
move_mixer_channel_left::reduce(state& st) const
{
    auto channel_ids = st.mixer_state.inputs.lock();

    auto it = std::ranges::find(*channel_ids, channel_id);
    BOOST_ASSERT(it != channel_ids->end());
    BOOST_ASSERT(it != channel_ids->begin());
    std::iter_swap(it, std::prev(it));
}

void
move_mixer_channel_right::reduce(state& st) const
{
    auto channel_ids = st.mixer_state.inputs.lock();

    auto it = std::ranges::find(*channel_ids, channel_id);
    BOOST_ASSERT(it != channel_ids->end());
    BOOST_ASSERT(std::next(it) != channel_ids->begin());
    std::iter_swap(it, std::next(it));
}

} // namespace piejam::runtime::actions
