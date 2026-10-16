// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/external_audio.h>

#include <piejam/audio/pair.h>

namespace piejam::runtime::external_audio
{

auto
get_channels_config(
    audio::bus_type const device_bus_type,
    device_channels_t const& device_channels,
    device_id id) -> channels_config_t
{
    if (device_bus_type == audio::bus_type::mono)
    {
        return device_channels.at({id, audio::bus_channel::mono});
    }

    return audio::pair<std::size_t>{
        device_channels.at({id, audio::bus_channel::left}),
        device_channels.at({id, audio::bus_channel::right})};
}

auto
get_channels_config(
    devices_t const& devices,
    device_channels_t const& device_channels,
    device_id id) -> channels_config_t
{
    return get_channels_config(devices.at(id).bus_type, device_channels, id);
}

} // namespace piejam::runtime::external_audio
