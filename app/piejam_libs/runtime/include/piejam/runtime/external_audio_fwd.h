// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/audio/fwd.h>
#include <piejam/fwd.h>

#include <boost/container/container_fwd.hpp>

#include <variant>
#include <vector>

namespace piejam::runtime::external_audio
{

struct device;
using device_id = entity_id<device>;
using devices_t = boxed_map<entity_map<device>>;

using device_ids_t = std::vector<device_id>;

struct device_channel_key;
using device_channels_t =
    boxed_map<boost::container::flat_map<device_channel_key, std::size_t>>;
using channels_config_t = std::variant<std::size_t, audio::pair<std::size_t>>;

struct state;

} // namespace piejam::runtime::external_audio
