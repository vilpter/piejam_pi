// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/entity_data_map.h>

#include <cstdint>

namespace piejam::runtime
{

// Material design colors
enum class material_color : std::uint8_t
{
    red,
    pink,
    purple,
    deep_purple,
    indigo,
    blue,
    light_blue,
    cyan,
    teal,
    green,
    light_green,
    lime,
    yellow,
    amber,
    orange,
    deep_orange,
    brown,
    grey,
    blue_grey,
};

using material_color_id = entity_id<struct material_color_id_tag>;

using material_colors_t = entity_data_map<material_color_id, material_color>;

} // namespace piejam::runtime
