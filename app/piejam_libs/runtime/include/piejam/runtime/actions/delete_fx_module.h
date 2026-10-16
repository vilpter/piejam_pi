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

struct delete_fx_module final
    : ui::cloneable_action<delete_fx_module, reducible_action>
{
    mixer::channel_id fx_chain_id;
    fx::module_id fx_mod_id;

    void reduce(state&) const override;
};

} // namespace piejam::runtime::actions
