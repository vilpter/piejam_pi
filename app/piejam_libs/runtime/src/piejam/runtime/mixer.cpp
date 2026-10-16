// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/mixer.h>

#include <piejam/runtime/bool_parameter.h>
#include <piejam/runtime/parameter/map.h>
#include <piejam/runtime/parameter/store.h>

#include <piejam/io_pair.h>

#include <algorithm>
#include <ranges>

namespace piejam::runtime::mixer
{

namespace
{

struct channel_io_t
{
    io_pair<io_address_t> port;
    std::vector<channel_id> aux_sends;
};

using channels_io_t = boost::container::flat_map<channel_id, channel_io_t>;

auto
extract_channels_io(
    io_map_t const& io_map,
    aux_sends_t const& aux_sends,
    parameter::store const& params) -> channels_io_t
{
    channels_io_t result;

    for (auto const& [id, io] : io_map)
    {
        auto [it, inserted] = result.emplace(id, io);
        BOOST_ASSERT(inserted);

        if (auto channel_aux_sends = aux_sends.find(id))
        {
            for (auto const& [aux_id, aux] : *channel_aux_sends)
            {
                if (params.at(aux.active()).get())
                {
                    it->second.aux_sends.emplace_back(aux_id);
                }
            }
        }
    }

    return result;
}

struct io_graph_node
{
    std::vector<channel_id> children;
    bool finished{};
    bool visited{};
};

using io_graph = boost::container::flat_map<channel_id, io_graph_node>;

auto
make_channels_io_graph(channels_io_t const& channels_io) -> io_graph
{
    io_graph result;
    result.reserve(channels_io.size());

    for (auto const& [id, ch_io] : channels_io)
    {
        result[id];

        if (channel_id const* const in_channel_id =
                std::get_if<channel_id>(&ch_io.port.in()))
        {
            result[*in_channel_id].children.push_back(id);
        }

        auto add_out_child = [&](channel_id const& out_channel_id) {
            if (std::holds_alternative<mixer::mix_input>(
                    channels_io.at(out_channel_id).port.in()))
            {
                result[id].children.push_back(out_channel_id);
            }
        };

        if (auto const* const out_channel_id =
                std::get_if<channel_id>(&ch_io.port.out()))
        {
            add_out_child(*out_channel_id);
        }

        for (auto aux : ch_io.aux_sends)
        {
            add_out_child(aux);
        }
    }

    return result;
}

bool
has_cycle(io_graph& g, channel_id const id)
{
    BOOST_ASSERT(g.contains(id));

    auto& node = g[id];

    if (node.finished)
    {
        return false;
    }

    if (node.visited)
    {
        return true;
    }

    node.visited = true;

    for (auto child : node.children)
    {
        if (has_cycle(g, child))
        {
            return true;
        }
    }

    node.finished = true;

    return false;
}

bool
has_cycle(io_graph g)
{
    return std::ranges::any_of(g | std::views::keys, [&g](auto id) {
        return has_cycle(g, id);
    });
}

} // namespace

auto
is_mix_input_valid(
    channel_id const ch_id,
    io_map_t const& io_map,
    aux_sends_t const& aux_sends,
    parameter::store const& params) -> bool
{
    auto channels_io = extract_channels_io(io_map, aux_sends, params);
    channels_io[ch_id].port.in() = mixer::mix_input{};
    return !has_cycle(make_channels_io_graph(channels_io));
}

auto
can_toggle_aux(
    channel_id const ch_id,
    channel_id const aux_id,
    io_map_t const& io_map,
    aux_sends_t const& aux_sends,
    parameter::store const& params) -> bool
{
    auto channel_aux_sends = aux_sends.find(ch_id);
    if (!channel_aux_sends)
    {
        return false;
    }

    auto aux_send = channel_aux_sends->find(aux_id);
    if (!aux_send)
    {
        return false;
    }

    if (params.at(aux_send->active()).get())
    {
        return true; // we can always disable an enabled aux
    }

    auto channels_io = extract_channels_io(io_map, aux_sends, params);

    channels_io[ch_id].aux_sends.emplace_back(aux_id);
    return !has_cycle(make_channels_io_graph(channels_io));
}

auto
disabled_by_routing_aux_sends(
    io_map_t const& io_map,
    aux_sends_t const& aux_sends,
    parameter::store const& params) -> std::vector<parameter_id>
{
    std::vector<parameter_id> result;

    auto channels_io = extract_channels_io(io_map, aux_sends, params);

    for (auto const& [ch_id, auxs] : aux_sends)
    {
        for (auto const& [aux_id, aux_send] : auxs)
        {
            if (params.at(aux_send.active()).get())
            {
                continue; // skip already enabled
            }

            channels_io[ch_id].aux_sends.emplace_back(aux_id);

            if (has_cycle(make_channels_io_graph(channels_io)))
            {
                result.emplace_back(aux_send.active());
            }

            channels_io[ch_id].aux_sends.pop_back();
        }
    }

    return result;
}

auto
valid_channels(
    channel_id const ch_id,
    io_direction const io_dir,
    channels_t const& channels,
    io_map_t const& io_map,
    aux_sends_t const& aux_sends,
    parameter::store const& params) -> std::vector<channel_id>
{
    auto channels_io = extract_channels_io(io_map, aux_sends, params);

    std::vector<mixer::channel_id> valid_ids;
    for (auto const& [mixer_channel_id, mixer_channel] : channels)
    {
        if (mixer_channel_id == ch_id)
        {
            // mono mixer channels can't have input channels
            if (io_dir == io_direction::input &&
                mixer_channel.type == channel_type::mono)
            {
                return {};
            }

            // otherwise, we can't be our own input
            continue;
        }

        if (io_dir == io_direction::output &&
            mixer_channel.type == channel_type::mono)
        {
            // mono mixer channels can't be targets
            continue;
        }

        auto prev_id =
            std::exchange(channels_io[ch_id].port[io_dir], mixer_channel_id);

        if (!has_cycle(make_channels_io_graph(channels_io)))
        {
            valid_ids.push_back(mixer_channel_id);
        }

        channels_io[ch_id].port[io_dir] = prev_id;
    }

    return valid_ids;
}

} // namespace piejam::runtime::mixer
