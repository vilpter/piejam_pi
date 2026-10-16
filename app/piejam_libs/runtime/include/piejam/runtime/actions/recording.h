// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/actions/recorder_action.h>
#include <piejam/runtime/fwd.h>
#include <piejam/runtime/ui/action.h>
#include <piejam/runtime/ui/cloneable_action.h>

namespace piejam::runtime::actions
{

struct start_recording final
    : ui::cloneable_action<start_recording, action>
    , visitable_recorder_action<start_recording>
{
};

struct stop_recording final
    : ui::cloneable_action<stop_recording, action>
    , visitable_recorder_action<stop_recording>
{
};

} // namespace piejam::runtime::actions
