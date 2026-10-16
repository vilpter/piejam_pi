// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/parameters.h>

#include <piejam/entity_id.h>
#include <piejam/midi/device_id.h>

#include <boost/container/container_fwd.hpp>

namespace piejam::runtime
{

struct midi_assignment
{
    std::size_t channel{};

    enum class type
    {
        cc,
        pitch_bend,
    } control_type{};

    std::size_t control_id{};

    constexpr auto
    operator<=>(midi_assignment const& other) const noexcept = default;
};

using midi_assignments_map =
    boost::container::flat_map<parameter_id, midi_assignment>;

} // namespace piejam::runtime
