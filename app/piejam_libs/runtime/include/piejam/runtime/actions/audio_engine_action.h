// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/actions/fwd.h>
#include <piejam/runtime/ui/action_visitor.h>

namespace piejam::runtime::actions
{

struct audio_engine_action_visitor
    : ui::action_visitor_interface<
          set_bool_parameter,
          set_float_parameter,
          set_int_parameter,
          set_enum_parameter,
          request_audio_engine_sync,
          request_info_update>
{
};

struct audio_engine_action
    : ui::visitable_action_interface<audio_engine_action_visitor>
{
};

template <class Action>
using visitable_audio_engine_action =
    ui::visitable_action<Action, audio_engine_action>;

} // namespace piejam::runtime::actions
