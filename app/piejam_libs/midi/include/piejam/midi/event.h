// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/midi/device_id.h>

#include <piejam/entity_id.h>

#include <cstddef>
#include <cstdint>
#include <variant>

namespace piejam::midi
{

struct cc_event
{
    std::uint8_t cc{};
    std::int8_t value{};

    constexpr auto operator==(cc_event const&) const noexcept -> bool = default;
};

struct pitch_bend_event
{
    std::int16_t value{};

    constexpr auto operator==(pitch_bend_event const&) const noexcept
        -> bool = default;
};

template <class E>
struct channel_event
{
    std::uint8_t channel{};
    E data{};

    constexpr auto operator==(channel_event const&) const noexcept
        -> bool = default;
};

using channel_cc_event = channel_event<cc_event>;
using channel_pitch_bend_event = channel_event<pitch_bend_event>;

using event_t = std::variant<channel_cc_event, channel_pitch_bend_event>;

struct external_event
{
    device_id_t device_id;
    event_t event{};

    constexpr auto operator==(external_event const&) const noexcept
        -> bool = default;
};

} // namespace piejam::midi
