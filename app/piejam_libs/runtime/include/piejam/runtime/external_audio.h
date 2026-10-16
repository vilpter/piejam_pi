// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/external_audio_fwd.h>
#include <piejam/runtime/string_id.h>

#include <piejam/audio/types.h>
#include <piejam/boxed_map.h>
#include <piejam/entity_map.h>
#include <piejam/io_pair.h>
#include <piejam/npos.h>

namespace piejam::runtime::external_audio
{

struct device
{
    string_id name;
    audio::bus_type bus_type;

    [[nodiscard]]
    constexpr auto operator==(device const&) const noexcept -> bool = default;
};

struct device_channel_key
{
    device_id id;
    audio::bus_channel ch;

    [[nodiscard]]
    constexpr auto
    operator<=>(device_channel_key const&) const noexcept = default;
};

auto get_channels_config(
    audio::bus_type device_bus_type,
    device_channels_t const& device_channels,
    device_id id) -> channels_config_t;

auto get_channels_config(
    devices_t const& devices,
    device_channels_t const& device_channels,
    device_id id) -> channels_config_t;

struct state
{
    devices_t devices;
    device_channels_t device_channels;

    io_pair<box<device_ids_t>> io_ids;
};

} // namespace piejam::runtime::external_audio
