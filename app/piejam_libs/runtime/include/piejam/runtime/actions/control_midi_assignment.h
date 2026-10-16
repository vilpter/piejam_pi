// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/fwd.h>
#include <piejam/runtime/midi_assignment.h>
#include <piejam/runtime/ui/action.h>
#include <piejam/runtime/ui/cloneable_action.h>

#include <optional>

namespace piejam::runtime::actions
{

struct start_midi_learning final
    : ui::cloneable_action<start_midi_learning, reducible_action>
{
    parameter_id assignment_id;

    void reduce(state&) const override;
};

struct stop_midi_learning final
    : ui::cloneable_action<stop_midi_learning, reducible_action>
{
    void reduce(state&) const override;

    std::optional<midi_assignment> learned;
};

struct clear_midi_assignment final
    : ui::cloneable_action<clear_midi_assignment, reducible_action>
{
    parameter_id assignment_id;

    void reduce(state&) const override;
};

} // namespace piejam::runtime::actions
