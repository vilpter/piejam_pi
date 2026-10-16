// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/external_audio_fwd.h>

#include <piejam/audio/types.h>
#include <piejam/default.h>
#include <piejam/fwd.h>

#include <boost/container/container_fwd.hpp>

#include <variant>
#include <vector>

namespace piejam::runtime::mixer
{

enum class channel_type
{
    mono,
    stereo,
    aux,
};

constexpr auto
to_bus_type(channel_type t) -> audio::bus_type
{
    return t == channel_type::mono ? audio::bus_type::mono
                                   : audio::bus_type::stereo;
}

struct channel;
using channel_id = entity_id<channel>;
using channels_t = boxed_map<entity_map<channel>>;

struct mix_input
{
    constexpr auto operator==(mix_input const&) const noexcept
        -> bool = default;
};

using io_address_t =
    std::variant<default_t, mix_input, external_audio::device_id, channel_id>;

using io_map_t =
    boxed_map<boost::container::flat_map<channel_id, io_pair<io_address_t>>>;

using channel_ids_t = std::vector<channel_id>;

struct aux_channel;
using aux_channels_t =
    boxed_map<boost::container::flat_map<channel_id, aux_channel>>;

struct aux_send;
using channel_aux_sends_t =
    lean_map_facade<boost::container::flat_map<channel_id, aux_send>>;
using aux_sends_t =
    boxed_map<boost::container::flat_map<channel_id, channel_aux_sends_t>>;

enum class aux_send_fader_tap
{
    auto_,
    post,
    pre,

    _min = auto_,
    _max = pre,
};

enum class aux_channel_fader_tap
{
    post,
    pre,

    _min = post,
    _max = pre,
};

struct state;

} // namespace piejam::runtime::mixer
