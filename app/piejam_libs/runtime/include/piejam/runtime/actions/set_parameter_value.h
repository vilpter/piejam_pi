// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/actions/audio_engine_action.h>
#include <piejam/runtime/fwd.h>
#include <piejam/runtime/parameters.h>
#include <piejam/runtime/ui/action.h>
#include <piejam/runtime/ui/cloneable_action.h>

#include <piejam/entity_id.h>

namespace piejam::runtime
{

template <class Parameter>
void set_parameter_value(
    state&,
    parameter::id_t<Parameter> id,
    parameter::value_type_t<Parameter> value);

namespace actions
{

template <class Parameter>
struct set_parameter_value final
    : ui::cloneable_action<set_parameter_value<Parameter>, reducible_action>
    , visitable_audio_engine_action<set_parameter_value<Parameter>>
{
    template <class V>
    set_parameter_value(parameter::id_t<Parameter> id, V&& value)
        : id(id)
        , value(std::forward<V>(value))
    {
    }

    void reduce(state&) const override;

    parameter::id_t<Parameter> id{};
    parameter::value_type_t<Parameter> value{};
};

template <class Parameter, class V>
set_parameter_value(parameter::id_t<Parameter>, V&&)
    -> set_parameter_value<Parameter>;

auto reset_parameter_to_default_value(parameter_id) -> thunk_action;

auto set_float_parameter_normalized(float_parameter_id, float norm_value)
    -> thunk_action;

} // namespace actions
} // namespace piejam::runtime
