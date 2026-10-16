// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/fwd.h>
#include <piejam/runtime/fx/fwd.h>
#include <piejam/runtime/mixer_fwd.h>
#include <piejam/runtime/ui/action.h>
#include <piejam/runtime/ui/cloneable_action.h>

#include <piejam/entity_id.h>

namespace piejam::runtime::actions
{

struct move_fx_module_up final
    : ui::cloneable_action<move_fx_module_up, reducible_action>
{
    void reduce(state&) const override;
};

struct move_fx_module_down final
    : ui::cloneable_action<move_fx_module_down, reducible_action>
{
    void reduce(state&) const override;
};

} // namespace piejam::runtime::actions
